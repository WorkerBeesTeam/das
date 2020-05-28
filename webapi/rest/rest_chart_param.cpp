#include <Das/log/log_param_item.h>

#include "rest_chart_param.h"

namespace Das {
namespace Rest {

std::string Chart_Param::permission_name() const
{
    return "view_log_param";
}

QString Chart_Param::get_table_name() const
{
    return Log_Param_Item::table_name();
}

QString Chart_Param::get_field_name(Chart_Value::Field_Type field_type) const
{
    static QStringList column_names = Log_Param_Item::table_column_names();

    switch (field_type)
    {
    case FT_TIME:       return column_names.at(Log_Param_Item::COL_timestamp_msecs);
    case FT_ITEM_ID:    return column_names.at(Log_Param_Item::COL_group_param_id);
    case FT_USER_ID:    return column_names.at(Log_Param_Item::COL_user_id);
    case FT_VALUE:      return column_names.at(Log_Param_Item::COL_value);
    case FT_SCHEME_ID:  return column_names.at(Log_Param_Item::COL_scheme_id);
    default:
        break;
    }
    return {};
}

QString Chart_Param::get_additional_field_names() const { return {}; }
void Chart_Param::fill_additional_fields(picojson::object &, const QSqlQuery &, const QVariant &) const {}

} // namespace Rest
} // namespace Das
