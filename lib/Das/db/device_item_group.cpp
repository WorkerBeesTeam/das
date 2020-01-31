#include "device_item_group.h"

namespace Das {
namespace Database {

Device_Item_Group::Device_Item_Group(uint32_t id, const QString& title, uint32_t section_id, uint32_t type_id, uint32_t parent_id) :
    id_(id), section_id_(section_id), type_id_(type_id), parent_id_(parent_id), title_(title)
{
}

uint32_t Device_Item_Group::id() const { return id_; }
void Device_Item_Group::set_id(uint32_t id) { id_ = id; }

QString Device_Item_Group::title() const { return title_; }
void Device_Item_Group::set_title(const QString& title) { title_ = title; }

uint32_t Device_Item_Group::section_id() const { return section_id_; }
void Device_Item_Group::set_section_id(uint32_t section_id) { section_id_ = section_id; }

uint32_t Device_Item_Group::type_id() const { return type_id_; }
void Device_Item_Group::set_type_id(uint32_t type_id) { type_id_ = type_id; }

uint32_t Device_Item_Group::parent_id() const { return parent_id_; }
void Device_Item_Group::set_parent_id(uint32_t parent_id) { parent_id_ = parent_id; }

QDataStream& operator>>(QDataStream& ds, Device_Item_Group& group)
{
    return ds >> group.id_ >> group.title_ >> group.section_id_ >> group.type_id_ >> group.parent_id_;
}

QDataStream& operator<<(QDataStream& ds, const Device_Item_Group& group)
{
    return ds << group.id() << group.title() << group.section_id() << group.type_id() << group.parent_id();
}

} // namespace Database
} // namespace Das
