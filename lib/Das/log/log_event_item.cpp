#include "log_event_item.h"

namespace Das {
namespace DB {

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

} // namespace DB
} // namespace Das
