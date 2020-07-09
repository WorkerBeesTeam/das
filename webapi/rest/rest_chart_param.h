#ifndef DAS_REST_CHART_PARAM_H
#define DAS_REST_CHART_PARAM_H

#include "rest_chart_value.h"

namespace Das {
namespace Rest {

class Chart_Param : public Chart_Value
{
public:
private:
    std::string permission_name() const override;
    QString get_table_name() const override;
    QString get_field_name(Field_Type field_type) const override;
    QString get_additional_field_names() const override;
    void fill_additional_fields(picojson::object&, const QSqlQuery&, const QVariant&) const override;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CHART_PARAM_H
