#include "disabled_param.h"

namespace Das {
namespace DB {

Disabled_Param::Disabled_Param(uint32_t id, uint32_t group_id, uint32_t param_id) :
    id_(id), group_id_(group_id), param_id_(param_id)
{
}

uint32_t Disabled_Param::id() const { return id_; }
void Disabled_Param::set_id(uint32_t id) { id_ = id; }

uint32_t Disabled_Param::group_id() const { return group_id_; }
void Disabled_Param::set_group_id(uint32_t group_id) { group_id_ = group_id; }

uint32_t Disabled_Param::param_id() const { return param_id_; }
void Disabled_Param::set_param_id(uint32_t param_id) { param_id_ = param_id; }

QDataStream &operator<<(QDataStream &ds, const Disabled_Param &item)
{
    return ds << item.id() << item.group_id() << item.param_id();
}

QDataStream &operator>>(QDataStream &ds, Disabled_Param &item)
{
    return ds >> item.id_ >> item.group_id_ >> item.param_id_;
}

} // namespace DB
} // namespace Das
