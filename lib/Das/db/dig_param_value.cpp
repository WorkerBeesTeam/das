#include "dig_param_value.h"

namespace Das {
namespace DB {

DIG_Param_Value_Base::DIG_Param_Value_Base(uint32_t group_param_id, const QString &value) :
    group_param_id_(group_param_id), value_(value)
{
}

uint32_t DIG_Param_Value_Base::group_param_id() const { return group_param_id_; }
void DIG_Param_Value_Base::set_group_param_id(uint32_t group_param_id) { group_param_id_ = group_param_id; }

QString DIG_Param_Value_Base::value() const { return value_; }
void DIG_Param_Value_Base::set_value(const QString &value) { value_ = value; }

QDataStream &operator<<(QDataStream &ds, const DIG_Param_Value_Base &item)
{
    return ds << item.group_param_id() << item.value();
}

QDataStream &operator>>(QDataStream &ds, DIG_Param_Value_Base &item)
{
    return ds >> item.group_param_id_ >> item.value_;
}

// -------------------------

DIG_Param_Value_Base2::DIG_Param_Value_Base2(qint64 timestamp_msecs, uint32_t user_id, uint32_t group_param_id, const QString &value) :
    Log_Base_Item(timestamp_msecs, user_id),
    DIG_Param_Value_Base(group_param_id, value)
{
}

QDataStream &operator<<(QDataStream &ds, const DIG_Param_Value_Base2 &item)
{
    return ds << static_cast<const Log_Base_Item&>(item) << static_cast<const DIG_Param_Value_Base&>(item);
}

QDataStream &operator>>(QDataStream &ds, DIG_Param_Value_Base2 &item)
{
    return ds >> static_cast<Log_Base_Item&>(item) >> static_cast<DIG_Param_Value_Base&>(item);
}

} // namespace DB
} // namespace Das
