#include "disabled_status.h"

namespace Das {
namespace DB {

Disabled_Status::Disabled_Status(uint32_t id, uint32_t group_id, uint32_t status_id) :
    id_(id), group_id_(group_id), status_id_(status_id)
{
}

uint32_t Disabled_Status::id() const { return id_; }
void Disabled_Status::set_id(uint32_t id) { id_ = id; }

uint32_t Disabled_Status::group_id() const { return group_id_; }
void Disabled_Status::set_group_id(uint32_t group_id) { group_id_ = group_id; }

uint32_t Disabled_Status::status_id() const { return status_id_; }
void Disabled_Status::set_status_id(uint32_t status_id) { status_id_ = status_id; }

QDataStream &operator<<(QDataStream &ds, const Disabled_Status &item)
{
    return ds << item.id() << item.group_id() << item.status_id();
}

QDataStream &operator>>(QDataStream &ds, Disabled_Status &item)
{
    return ds >> item.id_ >> item.group_id_ >> item.status_id_;
}

} // namespace DB
} // namespace Das
