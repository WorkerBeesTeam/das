#include "chart.h"

namespace Das {
namespace DB {

Chart_Item::Chart_Item(uint32_t id, const QString &name, const QString &color, uint32_t item_id, uint32_t param_id) :
    Base_Type(id, name),
    item_id_(item_id), param_id_(param_id),
    color_(color)
{
}

QString Chart_Item::color() const { return color_; }
void Chart_Item::set_color(const QString& color) { color_ = color; }

uint32_t Chart_Item::item_id() const { return item_id_; }
void Chart_Item::set_item_id(uint32_t item_id) { item_id_ = item_id; }

uint32_t Chart_Item::param_id() const { return param_id_; }
void Chart_Item::set_param_id(uint32_t param_id) { param_id_ = param_id; }

QDataStream &operator<<(QDataStream &ds, const Chart_Item &item)
{
    return ds << static_cast<const Base_Type&>(item) << item.color() << item.item_id() << item.param_id();
}

QDataStream &operator>>(QDataStream &ds, Chart_Item &item)
{
    return ds >> static_cast<Base_Type&>(item) >> item.color_ >> item.item_id_ >> item.param_id_;
}

} // namespace DB
} // namespace Das
