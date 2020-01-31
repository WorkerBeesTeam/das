#include <Das/commands.h>
#include <Das/db/device_item_value.h>

#include "server.h"
#include "database/db_thread_manager.h"
#include "database/db_scheme.h"
#include "server_protocol.h"
#include "dbus_object.h"

#include "log_synchronizer_2_0.h"

namespace Das {
namespace Ver_2_0 {
namespace Server {

Old::Log_Base_Item::Log_Base_Item(uint32_t id, qint64 time_msecs, uint32_t user_id) :
    id_(id), user_id_(user_id), time_msecs_(time_msecs) {}

uint32_t Old::Log_Base_Item::id() const { return id_; }

void Old::Log_Base_Item::set_id(uint32_t id) { id_ = id; }

qint64 Old::Log_Base_Item::time_msecs() const { return time_msecs_; }

void Old::Log_Base_Item::set_time_msecs(qint64 time_msecs) { time_msecs_ = time_msecs; }

QVariant Old::Log_Base_Item::date_to_db() const
{
    return QDateTime::fromMSecsSinceEpoch(time_msecs_, Qt::UTC);
}

void Old::Log_Base_Item::set_date_from_db(const QVariant &value)
{
    time_msecs_ = value.toDateTime().toUTC().toMSecsSinceEpoch();
}

uint32_t Old::Log_Base_Item::user_id() const { return user_id_; }

void Old::Log_Base_Item::set_user_id(uint32_t user_id) { user_id_ = user_id; }

QDataStream & Old::operator>>(QDataStream &ds, Old::Log_Base_Item &item)
{
    return ds >> item.id_ >> item.time_msecs_ >> item.user_id_;
}

QDataStream & Old::operator<<(QDataStream &ds, const Old::Log_Base_Item &item)
{
    return ds << item.id() << item.time_msecs() << item.user_id();
}

Old::Log_Value_Item::Log_Value_Item(uint32_t id, qint64 time_msecs, uint32_t user_id, uint32_t item_id, const QVariant &raw_value, const QVariant &value) :
    Old::Log_Base_Item(id, time_msecs, user_id),
    item_id_(item_id), raw_value_(raw_value), value_(value) {}

uint32_t Old::Log_Value_Item::item_id() const { return item_id_; }

void Old::Log_Value_Item::set_item_id(uint32_t item_id) { item_id_ = item_id; }

QVariant Old::Log_Value_Item::value() const { return value_; }

void Old::Log_Value_Item::set_value(const QVariant &value) { value_ = value; }

QVariant Old::Log_Value_Item::raw_value() const { return raw_value_; }

void Old::Log_Value_Item::set_raw_value(const QVariant &raw_value) { raw_value_ = raw_value; }

QDataStream &Old::operator<<(QDataStream &ds, const Old::Log_Value_Item &item)
{
    return ds << static_cast<const Old::Log_Base_Item&>(item) << item.item_id() << item.raw_value() << item.value();
}

QDataStream &Old::operator>>(QDataStream &ds, Old::Log_Value_Item &item)
{
    return ds >> static_cast<Old::Log_Base_Item&>(item) >> item.item_id_ >> item.raw_value_ >> item.value_;
}

Old::Log_Event_Item::Log_Event_Item(uint32_t id, qint64 time_msecs, uint32_t user_id, uint8_t type_id, const QString &category, const QString &text) :
    Old::Log_Base_Item(id, time_msecs, user_id),
    type_id_(type_id), category_(category), text_(text) {}

uint8_t Old::Log_Event_Item::type() const { return type_id_ & ~EVENT_NEED_TO_INFORM; }

void Old::Log_Event_Item::set_type(uint8_t type_id)
{
    type_id_ = type_id;
}

QString Old::Log_Event_Item::who() const { return category_; }

void Old::Log_Event_Item::set_who(QString category) { category_ = category; }

QString Old::Log_Event_Item::msg() const { return text_; }

void Old::Log_Event_Item::set_msg(QString text) { text_ = text; }

QString Old::Log_Event_Item::toString() const
{
    return "db_id = " + QString::number(id()) +
            " type_id = " + QString::number(type()) +
            " milliseconds = " + QString::number(time_msecs()) +
            " category = " + category_ +
            " message = " + text_;
}

bool Old::Log_Event_Item::need_to_inform() const
{
    return type_id_ & EVENT_NEED_TO_INFORM;
}

QDataStream &Old::operator<<(QDataStream &ds, const Old::Log_Event_Item &item)
{
    return ds << static_cast<const Old::Log_Base_Item&>(item)
              << uint8_t(item.type() | (item.need_to_inform() ? Old::Log_Event_Item::EVENT_NEED_TO_INFORM : 0))
              << item.who() << item.msg();
}

QDataStream &Old::operator>>(QDataStream &ds, Old::Log_Event_Item &item)
{
    return ds >> static_cast<Old::Log_Base_Item&>(item) >> item.type_id_ >> item.category_ >> item.text_;
}

LogSyncInfo::LogSyncInfo() :
    range_received_(false),
    pack_size_(OLD_LOG_SYNC_PACK_MAX_SIZE),
    change_size_(OLD_LOG_SYNC_PACK_MAX_SIZE), id(0), time(0), request_time_(0) {}

LogSyncInfo &LogSyncInfo::operator =(LogSyncInfo &&o) {
    range_received_ = std::move(o.range_received_);
    pack_size_ = std::move(o.pack_size_);
    change_size_ = std::move(o.change_size_);
    id = std::move(o.id);
    time = std::move(o.time);
    request_time_ = std::move(o.request_time_);
    ranges_ = std::move(o.ranges_);
    return *this;
}

bool LogSyncInfo::in_range(uint32_t id) const {
    for (const QPair<uint32_t, uint32_t>& range: ranges_)
        if (id >= range.first && id <= range.second)
            return true;
    return false;
}

time_t time_since_epoch()
{
    std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

QString old_log_table_name(uint8_t log_type, const QString &db_name)
{
    switch (static_cast<Log_Type>(log_type))
    {
    case LOG_VALUE: return Helpz::Database::db_table_name<Old::Log_Value_Item>(db_name);
    case LOG_EVENT: return Helpz::Database::db_table_name<Old::Log_Event_Item>(db_name);
    default: break;
    }
    return {};
}

Log_Synchronizer::Log_Synchronizer(Protocol_Base *protocol) :
    Base_Synchronizer(protocol)
{
}

void Log_Synchronizer::check()
{
    std::shared_ptr<Database::global> db_ptr = db();
    Helpz::Database::Table table{db_ptr->db_name(name()) + ".das_settings", {}, {"value"}};
    QString where = "WHERE param = '%1'";

    auto get_sync_info = [&](uint8_t log_type, LogSyncInfo& info)
    {
        auto q = db_ptr->select(table, where.arg(log_type == LOG_VALUE ? "sync_value_log" : "sync_event_log"));
        if (q.next()) {
            QStringList val = q.value(0).toString().split('|');
            if (val.size() == 2) {
                info.id = val.at(0).toUInt();
                info.time = val.at(1).toLongLong();
            }
        }

        if (!info.id)
        {
            q = db_ptr->select({old_log_table_name(log_type, db_ptr->db_name(name())), {}, {"remote_id", "date", "id"}}, "ORDER BY date DESC LIMIT 1");
            if (q.next())
            {
                info.id = q.value(0).toUInt();
                info.time = q.value(1).toDateTime().toMSecsSinceEpoch();
                if (!info.id)
                    q.value(2).toUInt();
            }
        }
    };

    std::unique_ptr<LogSyncInfo[]> info(new LogSyncInfo[2]);
    //    std::pair<LogSyncInfo, LogSyncInfo> result;
    get_sync_info(LOG_VALUE, info[0]);
    get_sync_info(LOG_EVENT, info[1]);

    qint64 value_log_time = info[0].time, event_log_time = info[1].time;
    sync_info_[LOG_VALUE] = std::move(info[0]);
    sync_info_[LOG_EVENT] = std::move(info[1]);
    request_log_range(LOG_VALUE, value_log_time);
    request_log_range(LOG_EVENT, event_log_time);
}

void Log_Synchronizer::check_unsync_log(Log_Type_Wrapper log_type, uint32_t db_id, qint64 time_ms)
{
    if (db_id && log_type.is_valid())
    {
        LogSyncInfo& info = sync_info_[log_type];
        if (!info.id || !info.range_received_ || info.ranges_.size())
        {
            return;
        }

        if (db_id == (info.id + 1))
        {
            ++info.id;
            info.time = time_ms;
        }
        else if (db_id > info.id)
            request_sync_seldom(log_type, false);
    }
}

QString Log_Synchronizer::valueLogSql(const QString &name)
{
    auto db_ptr = db();
    Helpz::Database::Table table = {Helpz::Database::db_table_name<Old::Log_Value_Item>(db_ptr->db_name(name)), {}, {"remote_id", "user_id", "date", "item_id", "raw_value", "value"}};
    return db_ptr->insert_query(table, table.field_names().size());
}

QString Log_Synchronizer::eventLogSql(const QString &name)
{
    auto db_ptr = db();
    Helpz::Database::Table table = {Helpz::Database::db_table_name<Old::Log_Event_Item>(db_ptr->db_name(name)), {}, {"remote_id", "user_id", "date", "type", "msg", "who"}};
    return db_ptr->insert_query(table, 6);
}

void Log_Synchronizer::deferred_eventLog(const QString &name, const QVector<Old::Log_Event_Item> &pack, std::function<void (uint32_t, qint64)> log_cb)
{
    if (pack.empty()) return;

    std::vector<QVariantList> values_pack;
    for (const Old::Log_Event_Item& item: pack)
    {
        values_pack.push_back(logValuesFromItem(item));
        log_cb(item.id(), item.time_msecs());
    }

    QString sql = eventLogSql(name);
    protocol()->db_thread()->add_pending_query(std::move(sql), std::move(values_pack));
}

void Log_Synchronizer::process_values_pack(const QVector<Old::Log_Value_Item> &pack)
{
    //        QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "device_item_values_available", Qt::QueuedConnection,
    //                                  Q_ARG(Scheme_Info, *protocol_), Q_ARG(QVector<Old::Log_Value_Item>, pack));

    deffered_setDevItemValue(name(), pack,
                             std::bind(&Log_Synchronizer::check_unsync_log, this, LOG_VALUE,
                                       std::placeholders::_1, std::placeholders::_2));

    request_sync_seldom(LOG_VALUE, true);
}

void Log_Synchronizer::process_events_pack(const QVector<Old::Log_Event_Item> &pack)
{
    deferred_eventLog(name(), pack,
                      std::bind(&Log_Synchronizer::check_unsync_log, this, LOG_EVENT,
                                std::placeholders::_1, std::placeholders::_2));

    //        QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "event_message_available", Qt::QueuedConnection,
    //                                  Q_ARG(Scheme_Info, *protocol_), Q_ARG(QVector<Old::Log_Event_Item>, pack));

    request_sync_seldom(LOG_EVENT, true);
}

void Log_Synchronizer::setLastLogSyncTime(const QString &name, const LogSyncInfo &info, const QString &paramName)
{
    auto db_ptr = db();
    QString sql = db_ptr->update_query({db_ptr->db_name(name) + ".das_settings", {}, {"value"}}, 1, "param = '" + paramName + '\'');
    std::vector<QVariantList> values_pack{{(QString::number(info.id) + '|' + QString::number(info.time))}};
    protocol()->db_thread()->add_pending_query(std::move(sql), std::move(values_pack));
}

void Log_Synchronizer::insertUnsyncData(const QString &name, const QVector<uint32_t> &not_found, const QVector<Old::Log_Value_Item> &data, LogSyncInfo *info)
{
    __insertUnsyncData(name, not_found, data, info);
}

void Log_Synchronizer::insertUnsyncData(const QString &name, const QVector<uint32_t> &not_found, const QVector<Old::Log_Event_Item> &data, LogSyncInfo *info)
{
    __insertUnsyncData(name, not_found, data, info);
}

template<typename T>
void Log_Synchronizer::__insertUnsyncData(const QString &name, const QVector<uint32_t>& not_found, const QVector<T>& data, LogSyncInfo *info)
{
    //    QString dbg_str = name + " [sync][" + (std::is_same<Old::Log_Value_Item, T>::value ? "values" : "events") + "] IUD ";
    //    std::cout << dbg_str
    //              << " notfound " << not_found.size()
    //              << " data " << data.size()
    //              << " info->id " << info->id << std::endl;
    if (data.empty()) return;

    uint32_t not_found_id_max = 0;
    for (uint32_t id: not_found)
        if (not_found_id_max < id)
            not_found_id_max = id;

    uint32_t min_val = -1, max_val = 0;
    std::vector<QVariantList> values_pack;
    for (const T& item: data)
    {
        if (min_val > item.id()) min_val = item.id();
        if (max_val < item.id()) max_val = item.id();
        if (!item.id() || !info->in_range(item.id()))
            continue;

        if (info->time <= item.time_msecs())
        {
            if (info->time < item.time_msecs())
                info->time = item.time_msecs();
            if (info->id < item.id())
                info->id = item.id();
        }
        values_pack.push_back(logValuesFromItem(item));
    }

    //    std::cout << dbg_str
    //              << "info->id " << info->id << ' ' << not_found_id_max
    //              << " values_pack " << values_pack.size()
    //              << " id min " << min_val << " max " << max_val << std::endl;
    if (!values_pack.size())
        return;

    if (info->id < not_found_id_max)
    {
        qWarning() << name << "Not found id is bigger then max id in pack" << not_found_id_max << ' ' << info->id;
        if (info->in_range(not_found_id_max))
            info->id = not_found_id_max;
    }

    info->ranges_.erase(std::remove_if(info->ranges_.begin(), info->ranges_.end(), [&info](QPair<uint32_t, uint32_t>& range) {
                            if (range.second <= info->id) {
                                //            std::cout << dbg_str << "remove range " << range.first << " " << range.second << std::endl;
                                return true;
                            } else if (range.first <= info->id) {
                                //            std::cout << dbg_str << "move range from " << range.first << " to " << (info->id + 1) << std::endl;
                                range.first = info->id + 1;
                            }
                            return false;
                        }), info->ranges_.end());

    if /*constexpr*/ (std::is_same<Old::Log_Value_Item, T>::value)
    {
        protocol()->db_thread()->add_pending_query(valueLogSql(name), std::move(values_pack));
        setLastLogSyncTime(name, *info, "sync_value_log");
    }
    else
    {
        protocol()->db_thread()->add_pending_query(eventLogSql(name), std::move(values_pack));
        setLastLogSyncTime(name, *info, "sync_event_log");
    }
}

void Log_Synchronizer::deffered_setDevItemValue(const QString &name, const QVector<Old::Log_Value_Item> &pack, std::function<void (uint32_t, qint64)> log_cb)
{
    if (pack.empty()) return;

    auto db_ptr = db();

    QString sql = db_ptr->insert_query(Helpz::Database::db_table<Device_Item_Value>(db_ptr->db_name(name)), 3, "ON DUPLICATE KEY UPDATE raw = VALUES(raw), display = VALUES(display)");

    std::vector<QVariantList> values_pack, log_values_pack;

    for (const Old::Log_Value_Item& item: pack)
    {
        values_pack.push_back({ item.item_id(),
                                Device_Item_Value::prepare_value(item.raw_value()),
                                Device_Item_Value::prepare_value(item.value()) });

        if (item.id())
        {
            log_values_pack.push_back(logValuesFromItem(item));
            log_cb(item.id(), item.time_msecs());
        }
    }

    protocol()->db_thread()->add_pending_query(std::move(sql), std::move(values_pack));
    if (!log_values_pack.empty())
        protocol()->db_thread()->add_pending_query(valueLogSql(name), std::move(log_values_pack));
}

QVariantList Log_Synchronizer::logValuesFromItem(const Old::Log_Value_Item &item)
{
    return QVariantList{
        !item.id() ? QVariant() : item.id(),
                !item.user_id() ? QVariant() : item.user_id(),
                QDateTime::fromMSecsSinceEpoch(item.time_msecs(), Qt::UTC),
                item.item_id(), item.raw_value(), item.value() };
}

QVariantList Log_Synchronizer::logValuesFromItem(const Old::Log_Event_Item &item)
{
    return QVariantList{
        !item.id() ? QVariant() : item.id(),
                !item.user_id() ? QVariant() : item.user_id(),
                QDateTime::fromMSecsSinceEpoch(item.time_msecs(), Qt::UTC),
                item.type(), item.msg(), item.who()
    };
}

void Log_Synchronizer::request_log_range(uint8_t log_type, qint64 log_time)
{
    protocol_->send(Cmd::LOG_RANGE_COUNT).answer([this](QIODevice& data_dev)
    {
        apply_parse(data_dev, &Log_Synchronizer::process_log_range);
    }).timeout([this]()
    {
        qWarning().noquote() << title() << "request_log_range timeout";
    }, std::chrono::seconds(15)) << log_type << log_time;
}

void Log_Synchronizer::request_sync_seldom(Log_Type_Wrapper log_type, bool continue_only)
{
    if (log_type.is_valid())
    {
        LogSyncInfo& info = sync_info_[log_type];
        if (continue_only && !info.ranges_.size())
        {
            return;
        }
        if (info.request_time_ == 0 || (time_since_epoch() - info.request_time_) > 30)
        {
            request_sync(info, log_type);
        }
    }
}

void Log_Synchronizer::request_sync(Log_Type_Wrapper log_type)
{
    if (log_type.is_valid())
    {
        request_sync(sync_info_[log_type], log_type);
    }
}

void Log_Synchronizer::request_sync(LogSyncInfo &info, uint8_t log_type)
{
    if (info.ranges_.empty())
    {
        // Если всё синхронизированно, обнуляем настройки
        info.pack_size_ = info.change_size_ = 250;
        info.request_time_ = 0;
        info.range_received_ = false;

        request_log_range(log_type, info.time);
    }
    else
    {
        std::time_t now = time_since_epoch();
        if (info.request_time_ != 0)
        {
            /* Если с последнего запроса прошло больше 30 секунд,
                 * Уменьшаем запрашиваемый пакет в двое,
                 * но пакет не должен быть меньше 3х.
                 * Пакет уменьшается вдвое пока не будет получен пакет,
                 * быстрее чем за 30 секунд. Если пакет получен быстрее
                 * и пакет меньше максимального (250), то величину
                 * изменения размера пакета устанавливаем в 3 значения,
                 * и следующий запрошенный пакет будет на 3 больше чем предыдущий. (НЕ ТОЧНО)*/

            if ((now - info.request_time_) > 30)
            {
                info.change_size_ = (info.change_size_ ? info.change_size_ : info.pack_size_) / 2;
                info.pack_size_ = std::max(7, info.pack_size_ - info.change_size_);
            }
            else if (info.pack_size_ < info.pack_max_size)
            {
                if (info.change_size_ > 3)
                    info.change_size_ = 3;
                info.pack_size_ = std::min(info.pack_max_size, static_cast<uint16_t>(info.pack_size_ + info.change_size_));
            }
        }
        info.request_time_ = now;

        QPair<uint32_t, uint32_t> range = *info.ranges_.begin();
        if ((range.second - range.first) > info.pack_size_)
        {
            range.second = range.first + info.pack_size_;
        }

        qDebug().noquote() << title() << "[sync][" << (log_type == LOG_VALUE ? "values" : "events") << "] Query from" << range.first << "to" << range.second;

        protocol_->send(Cmd::LOG_DATA).answer([this](QIODevice& data_dev)
        {
            apply_parse(data_dev, &Log_Synchronizer::process_log_data, &data_dev);
        }).timeout([this]() {
            qWarning().noquote() << title() << "request_sync timeout";
        }, std::chrono::seconds(15)) << log_type << range;
    }
}

std::vector<QPair<uint32_t, uint32_t> > Log_Synchronizer::checkSyncRange(const QString &name, uint8_t log_type, const QPair<uint32_t, uint32_t> &range, qint64 date_ms)
{
    if (range.first == 0 || range.second == 0)
        return {};

    auto db_ptr = db();
    QString where = "WHERE ";
    QVariantList values;

    if (date_ms > 0) {
        auto date = QDateTime::fromMSecsSinceEpoch(date_ms);
        if (date.isValid()) {
            where += "date > ? AND ";
            values.push_back(date);
        }
    }

    values.push_back(range.first);
    values.push_back(range.second);
    where += "remote_id >= ? AND remote_id <= ? ORDER BY remote_id ASC";
    auto q = db_ptr->select({old_log_table_name(log_type, db_ptr->db_name(name)), {}, {"remote_id"}}, where, values);
    if (!q.isActive())
        return {};

    std::vector<QPair<uint32_t, uint32_t>> result;
    if (!q.size())
        result.push_back(range);
    else
    {
        QPair<uint32_t, uint32_t> r = range;
        while(q.next())
        {
            r.second = q.value(0).toUInt() - 1;
            if (r.first <= r.second)
                result.push_back(r);
            r.first = ++r.second + 1;
        }
        if (r.second < range.second)
        {
            r.first = r.second + 1;
            r.second = range.second;
            result.push_back(r);
        }
    }
    return result;
}

void Log_Synchronizer::process_log_range(Log_Type_Wrapper log_type, const QPair<uint32_t, uint32_t> &range)
{
    if (log_type.is_valid())
    {
        LogSyncInfo& info = sync_info_[log_type];
        info.ranges_ = checkSyncRange(name(), log_type, range, info.time);
        info.range_received_ = true;

        qWarning().noquote() << title() << "[sync][" << (log_type == LOG_VALUE ? "values" : "events")
                             << "] Range from:" << range.first << "to:" << range.second;

        if (info.ranges_.size())
            request_sync(info, log_type);
        else if (range.first != 0 && range.second != 0 && range.second > info.id)
            info.id = range.second;
    }
}

void Log_Synchronizer::process_log_data(Log_Type_Wrapper log_type, const QVector<uint32_t> &not_found, QIODevice *data_dev)
{
    if (log_type.is_valid())
    {
        LogSyncInfo& info = sync_info_[log_type];

        switch (log_type)
        {
        case LOG_VALUE:
        {
            QVector<Old::Log_Value_Item> data;
            Helpz::parse_out(DATASTREAM_VERSION, *data_dev, data);
            qWarning().noquote() << title() << "[sync][values] Size: " << data.size()
                                 << "first id:" << (data.size() ? data.front().id() : 0)
                                 << "last id:" << (data.size() ? data.back().id() : 0);
            if (data.size())
                insertUnsyncData(name(), not_found, data, &info);
            else if (info.ranges_.size())
                info.ranges_.erase(info.ranges_.begin());
            break;
        }
        case LOG_EVENT:
        {
            QVector<Old::Log_Event_Item> data;
            Helpz::parse_out(DATASTREAM_VERSION, *data_dev, data);
            qWarning().noquote() << title() << "[sync][events] Size:" << data.size()
                                 << "first id:" << (data.size() ? data.front().id() : 0)
                                 << "last id:" << (data.size() ? data.back().id() : 0);
            if (data.size())
                insertUnsyncData(name(), not_found, data, &info);
            else if (info.ranges_.size())
                info.ranges_.erase(info.ranges_.begin());
            break;
        }
        default: break;
        }
        request_sync(info, log_type);
    }
}


} // namespace Server
} // namespace Ver_2_0
} // namespace Das
