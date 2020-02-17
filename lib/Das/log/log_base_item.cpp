#include <QDateTime>

#include "log_base_item.h"

namespace Das {

Q_LOGGING_CATEGORY(Sync_Log, "sync", QtInfoMsg)

namespace DB {

Log_Base_Item::Log_Base_Item(uint32_t id, qint64 timestamp_msecs, uint32_t user_id, bool flag) :
    flag_(flag), id_(id), user_id_(user_id), timestamp_msecs_(timestamp_msecs) {}

/*static*/ qint64 Log_Base_Item::current_timestamp()
{
    return QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
}

void Log_Base_Item::set_current_timestamp()
{
    timestamp_msecs_ = current_timestamp();
}

uint32_t Log_Base_Item::id() const { return id_; }
void Log_Base_Item::set_id(uint32_t id) { id_ = id; }

qint64 Log_Base_Item::timestamp_msecs() const { return timestamp_msecs_; }
void Log_Base_Item::set_timestamp_msecs(qint64 timestamp_msecs) { timestamp_msecs_ = timestamp_msecs; }

uint32_t Log_Base_Item::user_id() const { return user_id_; }
void Log_Base_Item::set_user_id(uint32_t user_id) { user_id_ = user_id; }

bool Log_Base_Item::flag() const { return flag_; }
void Log_Base_Item::set_flag(bool flag) { flag_ = flag; }

QDataStream &operator<<(QDataStream &ds, const Log_Base_Item &item)
{
    qint64 timestamp = item.timestamp_msecs();
    if (item.flag())
        timestamp |= Log_Base_Item::LOG_FLAG;
    return ds << timestamp << item.user_id();
}

QDataStream &operator>>(QDataStream &ds, Log_Base_Item &item)
{
    ds >> item.timestamp_msecs_ >> item.user_id_;

    item.flag_ = item.timestamp_msecs_ & Log_Base_Item::LOG_FLAG;
    item.timestamp_msecs_ &= ~Log_Base_Item::LOG_FLAG;

    return ds;
}

} // namespace DB
} // namespace Das
