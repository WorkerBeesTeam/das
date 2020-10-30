#include "device_item.h"

namespace Das {
namespace DB {

Device_Item::Device_Item(uint32_t id, const QString& name, uint32_t type_id, const QVariantList& extra_values,
                         uint32_t parent_id, uint32_t device_id, uint32_t group_id) :
    Named_Type(id, name), Device_Extra_Params(extra_values),
    type_id_(type_id), parent_id_(parent_id), device_id_(device_id), group_id_(group_id)
{
}

uint32_t Device_Item::type_id() const { return type_id_; }
void Device_Item::set_type_id(uint32_t type_id) { if (type_id_ != type_id) type_id_ = type_id; }

uint32_t Device_Item::parent_id() const { return parent_id_; }
void Device_Item::set_parent_id(uint32_t parent_id) { parent_id_ = parent_id; }

uint32_t Device_Item::device_id() const { return device_id_; }
void Device_Item::set_device_id(uint32_t device_id) { device_id_ = device_id; }

uint32_t Device_Item::group_id() const { return group_id_; }
void Device_Item::set_group_id(uint32_t group_id) { group_id_ = group_id; }

QDataStream &operator>>(QDataStream &ds, Device_Item &item)
{
    return ds >> static_cast<DB::Named_Type&>(item) >> item.type_id_ >> static_cast<Device_Extra_Params&>(item)
             >> item.parent_id_ >> item.device_id_ >> item.group_id_;
}

QDataStream &operator<<(QDataStream &ds, const Device_Item &item)
{
    return ds << static_cast<const DB::Named_Type&>(item) << item.type_id() << static_cast<const Device_Extra_Params&>(item)
              << item.parent_id() << item.device_id() << item.group_id();
}

} // namespace DB
} // namespace Das
