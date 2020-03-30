#ifndef DAS_DB_CHART_H
#define DAS_DB_CHART_H

#include <Helpz/db_meta.h>

#include "base_type.h"

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Chart : public Base_Type
{
    HELPZ_DB_META(Chart, "chart", "c", 3, DB_A(id), DB_A(name), DB_A(scheme_id))
public:
    using Base_Type::Base_Type;
};

// ----------------------------------------------------------------------------------------------------

class DAS_LIBRARY_SHARED_EXPORT Chart_Item : public Base_Type
{
    HELPZ_DB_META(Chart_Item, "chart_item", "ci", 6, DB_A(id), DB_A(chart_id), DB_A(color), DB_A(item_id), DB_A(param_id), DB_A(scheme_id))
public:
    Chart_Item(uint32_t id = 0, uint32_t chart_id = 0, const QString& color = {}, uint32_t item_id = 0, uint32_t param_id = 0);

    uint32_t chart_id() const;
    void set_chart_id(uint32_t chart_id);

    QString color() const;
    void set_color(const QString& id);

    uint32_t item_id() const;
    void set_item_id(uint32_t item_id);

    uint32_t param_id() const;
    void set_param_id(uint32_t param_id);
private:
    uint32_t chart_id_, item_id_, param_id_;

    friend QDataStream& operator>>(QDataStream& ds, Chart_Item& item);
};

QDataStream& operator<<(QDataStream& ds, const Chart_Item& item);
QDataStream& operator>>(QDataStream& ds, Chart_Item& item);

} // namespace DB
} // namespace Das

#endif // DAS_DB_CHART_H
