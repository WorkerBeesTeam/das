#include <QDateTime>

#include "log_pack.h"

namespace Das {

Q_LOGGING_CATEGORY(Sync_Log, "sync", QtInfoMsg)

Log_Base_Item::Log_Base_Item(uint32_t id, qint64 timestamp_msecs, uint32_t user_id, bool flag) :
    flag_(flag), id_(id), user_id_(user_id), timestamp_msecs_(timestamp_msecs) {}

void Log_Base_Item::set_current_timestamp()
{
    timestamp_msecs_ = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
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

// ------------------------------------------------------------------------------------------------

Log_Value_Item::Log_Value_Item(qint64 timestamp_msecs, uint32_t user_id, bool need_to_save, uint32_t item_id, const QVariant &raw_value, const QVariant &value) :
    Log_Base_Item(0, timestamp_msecs, user_id, need_to_save),
    item_id_(item_id), raw_value_(raw_value), value_(value) {}

uint32_t Log_Value_Item::item_id() const { return item_id_; }
void Log_Value_Item::set_item_id(uint32_t item_id) { item_id_ = item_id; }

QVariant Log_Value_Item::value() const { return value_; }
void Log_Value_Item::set_value(const QVariant &value) { value_ = value; }

QVariant Log_Value_Item::raw_value() const { return raw_value_; }
void Log_Value_Item::set_raw_value(const QVariant &raw_value) { raw_value_ = raw_value; }

bool Log_Value_Item::need_to_save() const { return flag_; }
void Log_Value_Item::set_need_to_save(bool state) { flag_ = state; }

QDataStream &operator<<(QDataStream &ds, const Log_Value_Item &item)
{
    return ds << static_cast<const Log_Base_Item&>(item) << item.item_id() << item.raw_value() << item.value();
}

QDataStream &operator>>(QDataStream &ds, Log_Value_Item &item)
{
    return ds >> static_cast<Log_Base_Item&>(item) >> item.item_id_ >> item.raw_value_ >> item.value_;
}

// ------------------------------------------------------------------------------------------------

Log_Event_Item::Log_Event_Item(qint64 timestamp_msecs, uint32_t user_id, bool need_to_inform, uint8_t type_id, const QString &category, const QString &text) :
    Log_Base_Item(0, timestamp_msecs, user_id, need_to_inform),
    type_id_(type_id), category_(category), text_(text) {}

uint8_t Log_Event_Item::type_id() const { return type_id_; }
void Log_Event_Item::set_type_id(uint8_t type_id) { type_id_ = type_id; }

QString Log_Event_Item::category() const { return category_; }
void Log_Event_Item::set_category(QString category) { category_ = category; }

QString Log_Event_Item::text() const { return text_; }
void Log_Event_Item::set_text(QString text) { text_ = text; }

bool Log_Event_Item::need_to_inform() const { return flag_; }
void Log_Event_Item::set_need_to_inform(bool state) { flag_ = state; }

QString Log_Event_Item::toString() const
{
    return  " type_id = " + QString::number(type_id()) +
            " milliseconds = " + QString::number(timestamp_msecs()) +
            " category = " + category_ +
            " message = " + text_;
}

QDataStream &operator<<(QDataStream &ds, const Log_Event_Item &item)
{
    return ds << static_cast<const Log_Base_Item&>(item) << item.type_id() << item.category() << item.text();
}

QDataStream &operator>>(QDataStream &ds, Log_Event_Item &item)
{
    return ds >> static_cast<Log_Base_Item&>(item) >> item.type_id_ >> item.category_ >> item.text_;
}

} // namespace Das
