#include "dig_param.h"

namespace Das {
namespace Database {

DIG_Param::DIG_Param(uint32_t id, uint32_t param_id, uint32_t group_id, uint32_t parent_id) :
    id_(id), param_id_(param_id), group_id_(group_id), parent_id_(parent_id)
{
}

uint32_t DIG_Param::id() const
{
    return id_;
}

void DIG_Param::set_id(uint32_t id)
{
    id_ = id;
}

uint32_t DIG_Param::param_id() const
{
    return param_id_;
}

void DIG_Param::set_param_id(const uint32_t &param_id)
{
    param_id_ = param_id;
}

uint32_t DIG_Param::group_id() const
{
    return group_id_;
}

void DIG_Param::set_group_id(const uint32_t &group_id)
{
    group_id_ = group_id;
}

uint32_t DIG_Param::parent_id() const
{
    return parent_id_;
}

void DIG_Param::set_parent_id(const uint32_t &parent_id)
{
    parent_id_ = parent_id;
}

QDataStream &operator<<(QDataStream &ds, const DIG_Param &item)
{
    return ds << item.id() << item.param_id() << item.group_id() << item.parent_id();
}

QDataStream &operator>>(QDataStream &ds, DIG_Param &item)
{
    return ds >> item.id_ >> item.param_id_ >> item.group_id_ >> item.parent_id_;
}

} // namespace Database
} // namespace Das
