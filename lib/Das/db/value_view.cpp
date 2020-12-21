#include "device_item_value.h"
#include "value_view.h"

namespace Das {
namespace DB {

Value_View::Value_View(uint32_t id, uint32_t type_id, const QVariant& value, const QVariant& view) :
    Base_Type{id},
    _type_id{type_id}, _value{value}, _view{view}
{
}

uint32_t Value_View::type_id() const { return _type_id; }
void Value_View::set_type_id(uint32_t type_id) { _type_id = type_id; }

QVariant Value_View::value() const { return _value; }
void Value_View::set_value(const QVariant &value) { _value = value; }

QVariant Value_View::value_to_db() const
{
    return Device_Item_Value::prepare_value(_value);
}

void Value_View::set_value_from_db(const QVariant &value)
{
    _value = Device_Item_Value::variant_from_string(value);
}

QVariant Value_View::view() const { return _view; }
void Value_View::set_view(const QVariant &view) { _view = view; }

QVariant Value_View::view_to_db() const
{
    return Device_Item_Value::prepare_value(_view);
}

void Value_View::set_view_from_db(const QVariant &view)
{
    _view = Device_Item_Value::variant_from_string(view);
}

QDataStream &operator>>(QDataStream &ds, Value_View &item)
{
    return ds >> static_cast<Base_Type&>(item) >> item._type_id >> item._value >> item._view;
}

QDataStream &operator<<(QDataStream &ds, const Value_View &item)
{
    return ds << static_cast<const Base_Type&>(item) << item.type_id() << item.value() << item.view();
}

} // namespace DB
} // namespace Das
