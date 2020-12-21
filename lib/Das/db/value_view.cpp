#include "device_item_value.h"
#include "value_view.h"

namespace Das {
namespace DB {

Value_View::Value_View(uint32_t item_type, const QVariant& value, const QVariant& view) :
    _item_type{item_type}, _value{value}, _view{view}
{
}

uint32_t Value_View::item_type() const { return _item_type; }
void Value_View::set_item_type(uint32_t item_type) { _item_type = item_type; }

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
    return ds >> item._item_type >> item._value >> item._view;
}

QDataStream &operator<<(QDataStream &ds, const Value_View &item)
{
    return ds << item.item_type() << item.value() << item.view();
}

} // namespace DB
} // namespace Das
