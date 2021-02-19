#include "chart.h"

namespace Das {
namespace DB {

Chart_Item::Chart_Item(uint32_t id, uint32_t chart_id, const QString &extra, uint32_t item_id, uint32_t param_id, uint32_t scheme_id) :
    Named_Type(id, extra, scheme_id),
    chart_id_(chart_id), item_id_(item_id), param_id_(param_id)
{
}

uint32_t Chart_Item::chart_id() const { return chart_id_; }
void Chart_Item::set_chart_id(uint32_t chart_id) { chart_id_ = chart_id; }

QString Chart_Item::extra() const { return name(); }
void Chart_Item::set_extra(const QString& data) { set_name(data); }

uint32_t Chart_Item::item_id() const { return item_id_; }
void Chart_Item::set_item_id(uint32_t item_id) { item_id_ = item_id; }

uint32_t Chart_Item::param_id() const { return param_id_; }
void Chart_Item::set_param_id(uint32_t param_id) { param_id_ = param_id; }

QDataStream &operator<<(QDataStream &ds, const Chart_Item &item)
{
    return ds << static_cast<const Named_Type&>(item) << item.chart_id() << item.item_id() << item.param_id();
}

QDataStream &operator>>(QDataStream &ds, Chart_Item &item)
{
    return ds >> static_cast<Named_Type&>(item) >> item.chart_id_ >> item.item_id_ >> item.param_id_;
}

} // namespace DB
} // namespace Das
