#include <Helpz/db_base.h>

#include <Das/db/dig_param_type.h>
#include <Das/db/dig_param_value.h>

#include "bot/scheme_item.h"

#include "dig_param.h"

namespace Das {
namespace Bot {
namespace User_Menu {

using namespace Helpz::DB;

std::string DIG_Param::get_value() const
{
    std::string result = Named_Data_Item::get_value();

    const QString sql =
            "SELECT dpv.value, dpt.value_type FROM das_dig_param dp "
            "LEFT JOIN das_dig_param_type dpt ON dpt.id = dp.param_id "
            "LEFT JOIN das_dig_param_value dpv ON dpv.group_param_id = dp.id AND dpv.scheme_id = "
            + QString::number(parameters().scheme_->id()) +
            " WHERE dp.scheme_id = "
            + QString::number(parameters().scheme_->parent_id_or_id()) +
            " AND dp.id = "
            + QString::number(id());

    Base& db = Base::get_thread_local_instance();
    auto q = db.exec(sql);
    if (q.next())
    {
        uint32_t value_type = q.value(1).toUInt();
        QVariant value = q.value(0);

        if (value_type == DB::DIG_Param_Type::VT_TIME)
        {
            uint32_t time = value.toUInt();
            if (time / 3600 < 10)
                result += '0';
            result += std::to_string(time / 3600);
            result += ':';
            if (time % 3600 / 60 < 10)
                result += '0';
            result += std::to_string(time % 3600 / 60);
            result += ':';
            if (time % 60 < 10)
                result += '0';
            result += std::to_string(time % 60);
        }
        else
            result += value.toString().toStdString();
    }
    else
        result += "Unknown";

    return result;
}

std::string DIG_Param::get_name() const
{
    const QString sql =
            "SELECT dptp.title, dpt.title FROM das_dig_param dp "
            "LEFT JOIN das_dig_param_type dpt ON dpt.id = dp.param_id "
            "LEFT JOIN das_dig_param_type dptp ON dptp.id = dpt.parent_id "
            " WHERE dpt.value_type != 7 AND dp.scheme_id = "
            + QString::number(parameters().scheme_->parent_id_or_id()) +
            " AND dp.id = "
            + QString::number(id());

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql);

    if (q.next())
    {
        std::string text = q.value(0).toString().toStdString();
        if (!text.empty())
            text += " - ";
        text += q.value(1).toString().toStdString();
    }

    return "Unknown";
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
