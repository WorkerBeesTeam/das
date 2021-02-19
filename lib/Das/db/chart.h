#ifndef DAS_DB_CHART_H
#define DAS_DB_CHART_H

#include <Helpz/db_meta.h>

#include "base_type.h"

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Chart : public Named_Type
{
    HELPZ_DB_META(Chart, "chart", "c", DB_A(id), DB_A(name), DB_A(scheme_id))
public:
    using Named_Type::Named_Type;
};

// ----------------------------------------------------------------------------------------------------

class DAS_LIBRARY_SHARED_EXPORT Chart_Item : public Named_Type
{
    HELPZ_DB_META(Chart_Item, "chart_item", "ci", DB_A(id), DB_A(chart_id), DB_A(extra), DB_AN(item_id), DB_AN(param_id), DB_A(scheme_id))
public:
    Chart_Item(uint32_t id = 0, uint32_t chart_id = 0, const QString& extra = {}, uint32_t item_id = 0, uint32_t param_id = 0, uint32_t scheme_id = Schemed_Model::default_scheme_id());

    uint32_t chart_id() const;
    void set_chart_id(uint32_t chart_id);

    QString extra() const;
    void set_extra(const QString& data);

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
