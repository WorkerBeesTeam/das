#include "device_item_type.h"

namespace Das {
namespace DB {

Device_Item_Type::Device_Item_Type(uint32_t id, const QString &name, const QString &title, uint32_t group_type_id,
                     uint32_t sign_id, uint8_t register_type, uint8_t save_algorithm, uint32_t save_timer_id) :
    Titled_Type(id, name, title),
    group_type_id(group_type_id), sign_id(sign_id),
    register_type(register_type), save_algorithm(save_algorithm), save_timer_id(save_timer_id) {}

QDataStream &operator<<(QDataStream &ds, const Device_Item_Type &item)
{
    return ds << static_cast<const Titled_Type&>(item) << item.group_type_id
              << item.sign_id << item.register_type << item.save_algorithm << item.save_timer_id;
}

QDataStream &operator>>(QDataStream &ds, Device_Item_Type &item)
{
    return ds >> static_cast<Titled_Type&>(item) >> item.group_type_id
              >> item.sign_id >> item.register_type >> item.save_algorithm >> item.save_timer_id;
}

uint Device_Item_Type_Manager::group_type_id(uint type_id) const
{
    return type(type_id).group_type_id;
}

uint Device_Item_Type_Manager::sign_id(uint type_id) const
{
    return type(type_id).sign_id;
}

uchar Device_Item_Type_Manager::register_type(uint type_id) const
{
    return type(type_id).register_type;
}

uchar Device_Item_Type_Manager::save_algorithm(uint type_id) const
{
    return type(type_id).save_algorithm;
}

uint32_t Device_Item_Type_Manager::save_timer_id(uint type_id) const
{
    const Device_Item_Type& device_item_type = type(type_id);
    return device_item_type.save_algorithm == Device_Item_Type::SA_BY_TIMER
           || device_item_type.save_algorithm == Device_Item_Type::SA_BY_TIMER_ANY_CASE
            ? device_item_type.save_timer_id : 0;
}

} // namespace DB
} // namespace Das
