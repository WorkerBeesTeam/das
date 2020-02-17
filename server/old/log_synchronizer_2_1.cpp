#include <Das/commands.h>
#include <Das/db/device_item_value.h>

#include "server.h"
#include "database/db_thread_manager.h"
#include "database/db_scheme.h"
#include "server_protocol.h"
#include "dbus_object.h"

#include "log_synchronizer_2_1.h"

namespace Das {
namespace Ver_2_1 {
namespace Server {

Log_Sync_Item::Log_Sync_Item(Log_Type type, Server::Protocol_Base *protocol) :
    Base_Synchronizer(protocol),
    in_progress_(false),
    type_(type)
{
}

void Log_Sync_Item::init_last_time()
{
    QString where = "param = '" + get_param_name() + '\'';
    auto db_ptr = db();
    Helpz::DB::Table table{db_ptr->db_name(name()) + ".das_settings", {}, {"value"}};

    auto q = db_ptr->select(table, "WHERE " + where);
    if (q.next())
        last_time_ = q.value(0).toString().toLongLong();
    else
        last_time_ = 0;
}


void Log_Sync_Item::check(qint64 request_time)
{
    if (!in_progress_)
    {
        in_progress_ = true;
        request_time_ = request_time;
        request_log_range_count();
    }
}

Helpz::DB::Thread *Log_Sync_Item::log_thread()
{
    return protocol()->work_object()->db_thread_mng_->log_thread();
}

QString Log_Sync_Item::log_insert_sql(const std::shared_ptr<DB::global>& db_ptr, const QString& name) const
{
    Helpz::DB::Table table = get_log_table(db_ptr, name);
    return db_ptr->insert_query(table, table.field_names().size(), {}, {}, "INSERT IGNORE");
}

void Log_Sync_Item::check_next()
{
    request_time_ = QDateTime::currentMSecsSinceEpoch();
    if (request_time_ - last_time_ > (15 * 60 * 1000))
        request_log_range_count();
    else
        in_progress_ = false;
}

void Log_Sync_Item::request_log_range_count()
{
    qCDebug(Sync_Log).noquote() << title() << t_str() << "request_log_range_count" << last_time_ << request_time_;
    protocol_->send(Cmd::LOG_RANGE_COUNT).answer([this](QIODevice& data_dev)
    {
        try
        {
            apply_parse(data_dev, &Log_Sync_Item::process_log_range_count, false);
            return; // in_progress_ is true
        }
        catch (const std::exception& e)
        {
            qCWarning(Sync_Log).noquote() << title() << t_str() << "receive bad range count" << e.what();
        }
        catch(...) {}

        in_progress_ = false;
    })
    .timeout([this]()
    {
        in_progress_ = false;
        qCWarning(Sync_Log).noquote() << title() << t_str() << "request_log_range timeout";
    }, std::chrono::seconds(15)) << type_ << last_time_ << request_time_;
}

bool Log_Sync_Item::process_log_range_count(uint32_t remote_count, bool just_check)
{
    qCDebug(Sync_Log).noquote() << title() << t_str() << "process_log_range_count" << last_time_ << request_time_ << remote_count << just_check;
    auto db_ptr = db();
    uint32_t count = db_ptr->row_count(log_table_name(type_, db_ptr->db_name(name())),
                                       "timestamp_msecs >= " + QString::number(last_time_) +
                                       " AND timestamp_msecs <= " + QString::number(request_time_));
    if (count >= remote_count)
    {
        if (count > remote_count)
        {
            qCWarning(Sync_Log).noquote() << title() << t_str() << "Server have more log values then client" << count << remote_count;
        }

        save_sync_time(db_ptr);
        check_next();
    }
    else if (just_check)
    {
        return false;
    }
    else if (remote_count > LOG_SYNC_PACK_MAX_SIZE)
    {
        request_time_ = last_time_ + ((request_time_ - last_time_) / 2);
        request_log_range_count();
    }
    else
    {
        request_log_data();
    }

    return true;
}

void Log_Sync_Item::save_sync_time(const std::shared_ptr<DB::global>& db_ptr)
{
    last_time_ = request_time_;
    qCDebug(Sync_Log).noquote() << title() << t_str() << "save_sync_time" << last_time_;

    Helpz::DB::Table table{db_ptr->db_name(name()) + ".das_settings", {}, {"param", "value"}};
    QString sql = db_ptr->insert_query(table, 2, "ON DUPLICATE KEY UPDATE value = VALUES(value)");

    std::vector<QVariantList> values_pack{{get_param_name(), QString::number(last_time_)}};
    log_thread()->add_pending_query(std::move(sql), std::move(values_pack));
}

void Log_Sync_Item::request_log_data()
{
    qCDebug(Sync_Log).noquote() << title() << t_str() << "request_log_data" << last_time_ << request_time_;
    protocol_->send(Cmd::LOG_DATA).answer([this](QIODevice& data_dev)
    {
        process_log_data(data_dev);
    })
    .timeout([this]()
    {
        in_progress_ = false;
        qCWarning(Sync_Log).noquote() << title() << t_str() << "request_sync timeout";
    }, std::chrono::seconds(15)) << type_ << last_time_ << request_time_;
}

void Log_Sync_Item::process_log_data(QIODevice& data_dev)
{
    qCDebug(Sync_Log).noquote() << title() << t_str() << "process_log_data" << last_time_ << request_time_;
    QString sql;
    QVariantList values_pack;
    int row_count = 0;

    try
    {
        fill_log_data(data_dev, sql, values_pack, row_count);
    }
    catch (const std::exception& e)
    {
        qCWarning(Sync_Log).noquote() << title() << t_str() << "receive bad data" << e.what();
    }
    catch(...) {}

    if (!row_count)
    {
        qCWarning(Sync_Log).noquote() << title() << t_str() << "receive empty data";
    }
    else
    {
        std::vector<QVariantList> packs;
        packs.push_back(std::move(values_pack));
        log_thread()->add_pending_query(std::move(sql), std::move(packs));
        if (process_log_range_count(row_count, true))
        {
            return; // in_progress_ is true
        }

        qCWarning(Sync_Log).noquote() << title() << t_str() << "receive diffrent data count";
    }

    in_progress_ = false;
}

// ------------------------------------------------------------------------------------------
QString get_q_array(const Helpz::DB::Table& table, int row_count)
{
    return "INSERT IGNORE INTO " + table.name() + '(' + table.field_names().join(',') + ") VALUES" +
            Helpz::DB::Base::get_q_array(table.field_names().size(), row_count);
}

Log_Sync_Values::Log_Sync_Values(Protocol_Base *protocol) :
    Log_Sync_Item(LOG_VALUE, protocol)
{
}

void Log_Sync_Values::process_pack(const QVector<Log_Value_Item>& pack)
{
    if (pack.empty()) return;

    QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "device_item_values_available", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *protocol_), Q_ARG(QVector<Log_Value_Item>, pack));

    std::vector<QVariantList> values_pack;
    QVariantList log_values_pack;
    int row_count = 0;
    for (const Log_Value_Item& item: pack)
    {
        values_pack.push_back({ item.item_id(),
                                Device_Item_Value::prepare_value(item.raw_value()),
                                Device_Item_Value::prepare_value(item.value()) });

        if (item.need_to_save())
        {
            ++row_count;
            log_values_pack += Log_Value_Item::to_variantlist(item);
        }
    }

    auto db_ptr = db();
    QString sql = db_ptr->insert_query(Helpz::DB::db_table<Device_Item_Value>(DB::global::db_name(name())), 3, "ON DUPLICATE KEY UPDATE raw = VALUES(raw), display = VALUES(display)");
    log_thread()->add_pending_query(std::move(sql), std::move(values_pack));

    if (row_count)
    {
        auto table = Helpz::DB::db_table<Log_Value_Item>(DB::global::db_name(name()));
        log_thread()->add_pending_query(get_q_array(table, row_count), {log_values_pack});
//        log_thread()->add_pending_query(log_insert_sql(db_ptr, name()), std::move(log_values_pack));
    }
}

QString Log_Sync_Values::t_str() const
{
    return "[V]";
}

Helpz::DB::Table Log_Sync_Values::get_log_table(const std::shared_ptr<DB::global>& db_ptr, const QString& name) const
{
    return Helpz::DB::db_table<Log_Value_Item>(db_ptr->db_name(name));
}

QString Log_Sync_Values::get_param_name() const
{
    return "sync_time_of_value_log";
}

void Log_Sync_Values::fill_log_data(QIODevice& data_dev, QString &sql, QVariantList &values_pack, int &row_count)
{
    QVector<Log_Value_Item> data;
    Helpz::parse_out(DATASTREAM_VERSION, data_dev, data);
    row_count = data.size();
    for (const Log_Value_Item& item: data)
        values_pack += Log_Value_Item::to_variantlist(item);

    auto table = Helpz::DB::db_table<Log_Value_Item>(DB::global::db_name(name()));
    sql = get_q_array(table, data.size());
}

// ------------------------------------------------------------------------------------------

Log_Sync_Events::Log_Sync_Events(Protocol_Base *protocol) :
    Log_Sync_Item(LOG_EVENT, protocol)
{
}

void Log_Sync_Events::process_pack(const QVector<Log_Event_Item>& pack)
{
    if (pack.empty()) return;

    QVariantList values_pack;
    for (const Log_Event_Item& item: pack)
        values_pack += Log_Event_Item::to_variantlist(item);

//    auto db_ptr = db();
    auto table = Helpz::DB::db_table<Log_Event_Item>(DB::global::db_name(name()));
    log_thread()->add_pending_query(get_q_array(table, pack.size()), {values_pack});
//    log_thread()->add_pending_query(log_insert_sql(db_ptr, name()), std::move(values_pack));

    QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "event_message_available", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *protocol_), Q_ARG(QVector<Log_Event_Item>, pack));
}

QString Log_Sync_Events::t_str() const
{
    return "[E]";
}

Helpz::DB::Table Log_Sync_Events::get_log_table(const std::shared_ptr<DB::global>& db_ptr, const QString& name) const
{
    return Helpz::DB::db_table<Log_Event_Item>(db_ptr->db_name(name));
}

QString Log_Sync_Events::get_param_name() const
{
    return "sync_time_of_event_log";
}

void Log_Sync_Events::fill_log_data(QIODevice& data_dev, QString &sql, QVariantList &values_pack, int& row_count)
{
    QVector<Log_Event_Item> data;
    Helpz::parse_out(DATASTREAM_VERSION, data_dev, data);
    row_count = data.size();
    for (const Log_Event_Item& item: data)
        values_pack += Log_Event_Item::to_variantlist(item);

    auto table = Helpz::DB::db_table<Log_Event_Item>(DB::global::db_name(name()));
    sql = get_q_array(table, data.size());
}

// ------------------------------------------------------------------------------------------

Log_Synchronizer::Log_Synchronizer(Protocol_Base *protocol) :
    values_(protocol), events_(protocol)
{
}

void Log_Synchronizer::init_last_time()
{
    values_.init_last_time();
    events_.init_last_time();
}

void Log_Synchronizer::check()
{
    qint64 request_time = QDateTime::currentMSecsSinceEpoch();
    values_.check(request_time);
    events_.check(request_time);
}

} // namespace Server
} // namespace Ver_2_1
} // namespace Das
