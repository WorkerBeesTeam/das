#include <Helpz/dtls_server_node.h>

#include <Das/commands.h>
#include <Das/db/device_item_value.h>

#include "server.h"
#include "database/db_thread_manager.h"
#include "database/db_scheme.h"
#include "server_protocol.h"
#include "dbus_object.h"

#include "log_synchronizer.h"

namespace Das {
namespace Ver {
namespace Server {

using namespace Helpz::DB;

Log_Sync_Item::Log_Sync_Item(Log_Type type, Protocol_Base *protocol) :
    Base_Synchronizer(protocol),
    in_progress_(false),
    type_(type)
{
}

void Log_Sync_Item::check()
{
    if (!in_progress_)
    {
        in_progress_ = true;
        request_log_data();
    }
}

Thread *Log_Sync_Item::log_thread()
{
    return protocol()->work_object()->db_thread_mng_->log_thread();
}

void Log_Sync_Item::request_log_data()
{
    qCDebug(Sync_Log).noquote() << title() << type_.to_string() << "request_log_data";
    protocol_->send(Cmd::LOG_DATA_REQUEST).answer([this](QIODevice& /*data_dev*/)
    {
        in_progress_ = false;
    })
    .timeout([this]()
    {
        in_progress_ = false;
        qCWarning(Sync_Log).noquote() << title() << type_.to_string() << "request_log_range timeout";
    }, std::chrono::seconds(15)) << type_;
}

void Log_Sync_Item::process_log_data(QIODevice& data_dev, uint8_t msg_id)
{
    qCDebug(Sync_Log).noquote() << title() << type_.to_string() << "process_log_data";
    QString sql;
    QVariantList values_pack;
    int row_count = 0;

    try
    {
        fill_log_data(data_dev, sql, values_pack, row_count);
    }
    catch (const std::exception& e)
    {
        qCWarning(Sync_Log).noquote() << title() << type_.to_string() << "receive bad data" << e.what();
    }
    catch(...) {}

    if (!row_count)
    {
        qCWarning(Sync_Log).noquote() << title() << type_.to_string() << "receive empty data";
    }
    else
    {
        auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());
        if (!node)
        {
            qWarning(Struct_Log).noquote() << title() << "Bad node pointer process_log_data";
            return;
        }

        Log_Type_Wrapper type = type_;
        log_thread()->add([msg_id, sql, values_pack, node, type](Base* db)
        {
            std::shared_ptr<Protocol> scheme = std::dynamic_pointer_cast<Protocol>(node->protocol());

            if (db->exec(sql, values_pack).isActive())
            {
                if (scheme)
                {
                    scheme->send_answer(Cmd::LOG_DATA_REQUEST, msg_id);
                }
            }

            if (scheme && type.is_valid())
            {
                scheme->log_sync()->log_sync_item(type)->request_log_data();
            }
        });
    }

    in_progress_ = false;
}

// ------------------------------------------------------------------------------------------
QString get_custom_q_array(const Table& table, int row_count)
{
    // " + (force ? QString("IGNORE ") : QString()) + "
    return "INSERT INTO " +
            table.name() + '(' + table.field_names().join(',') + ") VALUES" +
            Base::get_q_array(table.field_names().size(), row_count);
}

Log_Sync_Values::Log_Sync_Values(Protocol_Base *protocol) :
    Log_Sync_Item(LOG_VALUE, protocol)
{
}

void Log_Sync_Values::process_pack(QVector<Log_Value_Item> &&pack, uint8_t msg_id)
{
    if (pack.empty())
    {
        protocol_->send_answer(Cmd::LOG_PACK_VALUES, msg_id);
        return;
    }

    uint32_t s_id = scheme_id();

    QVariantList values_pack, tmp_values;
    int row_count = 0;
    for (Log_Value_Item& item: pack)
    {
        static_cast<Protocol*>(protocol())->structure_sync()->change_devitem_value(item);

        if (item.need_to_save())
        {
            ++row_count;

            item.set_scheme_id(s_id);
            tmp_values = Log_Value_Item::to_variantlist(item);
            tmp_values.removeFirst();

            values_pack += tmp_values;
        }
    }

    if (values_pack.empty())
    {
        protocol_->send_answer(Cmd::LOG_PACK_VALUES, msg_id);
    }
    else
    {
        auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());

        log_thread()->add([values_pack, s_id, node, msg_id](Base* db)
        {
            auto table = db_table<Log_Value_Item>();
            table.field_names().removeFirst(); // remove id

            const QString sql = get_custom_q_array(table, values_pack.size() / table.field_names().size());
            if (db->exec(sql, values_pack).isActive(), node)
            {
                auto protocol = node->protocol();
                if (protocol)
                {
                    protocol->send_answer(Cmd::LOG_PACK_VALUES, msg_id);
                }
            }
        });
    }

    QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "device_item_values_available", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *protocol_), Q_ARG(QVector<Log_Value_Item>, pack));
}

QString Log_Sync_Values::get_param_name() const
{
    return "sync_time_of_value_log";
}

void Log_Sync_Values::fill_log_data(QIODevice& data_dev, QString &sql, QVariantList &values_pack, int &row_count)
{
    QVariantList tmp_values;
    QVector<Log_Value_Item> data;
    Helpz::parse_out(DATASTREAM_VERSION, data_dev, data);
    row_count = data.size();
    for (Log_Value_Item& item: data)
    {
        item.set_scheme_id(scheme_id());
        tmp_values = Log_Value_Item::to_variantlist(item);
        tmp_values.removeFirst();
        values_pack += tmp_values;
    }

    auto table = db_table<Log_Value_Item>();
    table.field_names().removeFirst(); // remove id
    sql = get_custom_q_array(table, data.size());
}

// ------------------------------------------------------------------------------------------

Log_Sync_Events::Log_Sync_Events(Protocol_Base *protocol) :
    Log_Sync_Item(LOG_EVENT, protocol)
{
}

void Log_Sync_Events::process_pack(QVector<Log_Event_Item>&& pack, uint8_t msg_id)
{
    if (pack.empty())
    {
        protocol_->send_answer(Cmd::LOG_PACK_EVENTS, msg_id);
        return;
    }

    uint32_t s_id = scheme_id();

    QVariantList values_pack, tmp_values;
    for (Log_Event_Item& item: pack)
    {
        item.set_scheme_id(s_id);
        tmp_values = Log_Event_Item::to_variantlist(item);
        tmp_values.removeFirst();

        values_pack += tmp_values;
    }

    auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());

    log_thread()->add([values_pack, s_id, node, msg_id](Base* db)
    {
        auto table = db_table<Log_Event_Item>();
        table.field_names().removeFirst(); // remove id
        QString sql = get_custom_q_array(table, values_pack.size() / table.field_names().size());

        if (db->exec(sql, values_pack).isActive(), node)
        {
            auto protocol = node->protocol();
            if (protocol)
            {
                protocol->send_answer(Cmd::LOG_PACK_EVENTS, msg_id);
            }
        }
    });

    QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "event_message_available", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *protocol_), Q_ARG(QVector<Log_Event_Item>, pack));
}

QString Log_Sync_Events::get_param_name() const
{
    return "sync_time_of_event_log";
}

void Log_Sync_Events::fill_log_data(QIODevice& data_dev, QString &sql, QVariantList &values_pack, int& row_count)
{
    QVariantList tmp_values;
    QVector<Log_Event_Item> data;
    Helpz::parse_out(DATASTREAM_VERSION, data_dev, data);
    row_count = data.size();
    for (Log_Event_Item& item: data)
    {
        item.set_scheme_id(scheme_id());
        tmp_values = Log_Event_Item::to_variantlist(item);
        tmp_values.removeFirst();
        values_pack += tmp_values;
    }

    auto table = db_table<Log_Event_Item>();
    table.field_names().removeFirst(); // remove id
    sql = get_custom_q_array(table, data.size());
}

// ------------------------------------------------------------------------------------------

Log_Synchronizer::Log_Synchronizer(Protocol_Base *protocol) :
    values_(protocol), events_(protocol)
{
}

void Log_Synchronizer::check()
{
    values_.check();
    events_.check();
}

void Log_Synchronizer::process_data(Log_Type_Wrapper type_id, QIODevice *data_dev, uint8_t msg_id)
{
    switch (type_id.value())
    {
    case LOG_VALUE:
        values_.process_log_data(*data_dev, msg_id);
        break;
    case LOG_VALUE:
        events_.process_log_data(*data_dev, msg_id);
        break;
    default:
        break;
    }
}

void Log_Synchronizer::process_pack(Log_Type_Wrapper type_id, QIODevice *data_dev, uint8_t msg_id)
{
    switch (type_id.value())
    {
    case LOG_VALUE:
        Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Log_Sync_Values::process_pack, &values_, msg_id);
        break;
    case LOG_VALUE:
        Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Log_Sync_Events::process_pack, &events_, msg_id);
        break;
    default:
        break;
    }
}

Log_Sync_Item *Log_Synchronizer::log_sync_item(uint8_t type_id)
{
    if (type_id == LOG_VALUE)
    {
        return &values_;
    }
    else if (type_id == LOG_EVENT)
    {
        return &events_;
    }
    return nullptr;
}

} // namespace Server
} // namespace Ver
} // namespace Das
