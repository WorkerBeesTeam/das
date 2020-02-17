#include "dig_param_value.h"

namespace Das {
namespace DB {

// -------------------------

DIG_Param_Value::DIG_Param_Value(qint64 timestamp_msecs, uint32_t user_id, uint32_t group_param_id, const QString &value) :
    Log_Base_Item(0, timestamp_msecs, user_id),
    group_param_id_(group_param_id), value_(value)
{
}

uint32_t DIG_Param_Value::group_param_id() const { return group_param_id_; }
void DIG_Param_Value::set_group_param_id(uint32_t group_param_id) { group_param_id_ = group_param_id; }

QString DIG_Param_Value::value() const { return value_; }
void DIG_Param_Value::set_value(const QString &value) { value_ = value; }

QDataStream &operator<<(QDataStream &ds, const DIG_Param_Value &item)
{
    return ds << static_cast<const Log_Base_Item&>(item) << item.group_param_id() << item.value();
}

QDataStream &operator>>(QDataStream &ds, DIG_Param_Value &item)
{
    return ds >> static_cast<Log_Base_Item&>(item) >> item.group_param_id_ >> item.value_;
}

} // namespace DB
} // namespace Das
