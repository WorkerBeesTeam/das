#include "disabled_status.h"

namespace Das {
namespace DB {

Disabled_Status::Disabled_Status(uint32_t id, uint32_t group_id, uint32_t dig_id, uint32_t status_id) :
    _id(id), _group_id(group_id), _dig_id(dig_id), _status_id(status_id)
{
}

uint32_t Disabled_Status::id() const { return _id; }
void Disabled_Status::set_id(uint32_t id) { _id = id; }

uint32_t Disabled_Status::group_id() const { return _group_id; }
void Disabled_Status::set_group_id(uint32_t group_id) { _group_id = group_id; }

uint32_t Disabled_Status::dig_id() const { return _dig_id; }
void Disabled_Status::set_dig_id(uint32_t id) {   _dig_id = id; }

uint32_t Disabled_Status::status_id() const { return _status_id; }
void Disabled_Status::set_status_id(uint32_t status_id) { _status_id = status_id; }

QDataStream &operator<<(QDataStream &ds, const Disabled_Status &item)
{
    return ds << item.id() << item.group_id() << item.dig_id() << item.status_id();
}

QDataStream &operator>>(QDataStream &ds, Disabled_Status &item)
{
    return ds >> item._id >> item._group_id >> item._dig_id >> item._status_id;
}

} // namespace DB
} // namespace Das
