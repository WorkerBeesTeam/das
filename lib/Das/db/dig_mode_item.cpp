#include "dig_mode_item.h"

namespace Das {
namespace Database {

DIG_Mode_Item::DIG_Mode_Item(uint32_t group_id, uint32_t mode_id) :
    id_(0), group_id_(group_id), mode_id_(mode_id)
{
}

uint32_t DIG_Mode_Item::id() const { return id_; }
void DIG_Mode_Item::set_id(uint32_t id) { id_ = id; }

uint32_t DIG_Mode_Item::group_id() const { return group_id_; }
void DIG_Mode_Item::set_group_id(uint32_t group_id) { group_id_ = group_id; }

uint32_t DIG_Mode_Item::mode_id() const { return mode_id_; }
void DIG_Mode_Item::set_mode_id(uint32_t mode_id) { mode_id_ = mode_id; }

QDataStream &operator>>(QDataStream &ds, DIG_Mode_Item &item)
{
    return ds >> item.group_id_ >> item.mode_id_;
}

QDataStream &operator<<(QDataStream &ds, const DIG_Mode_Item &item)
{
    return ds << item.group_id() << item.mode_id();
}

} // namespace Database
} // namespace Das
