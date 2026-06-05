#include <Helpz/db_base.h>

#include <Das/db/device_item_value.h>

#include "bot/scheme_item.h"

#include "device_item_value_normalizer.h"
#include "device_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

using namespace Helpz::DB;

Device_Item::Device_Item()
{
}

std::string Device_Item::get_value() const
{
    std::string result = Named_Data_Item::get_value();

    const Value_Pair p = get_value_pair();
    if (p.value_.isValid())
    {
        if (normalizer_)
            result += normalizer_->normalize(p.value_.toDouble());
        else
            result += p.value_.toString().toStdString();

        if (!p.sign_.empty())
            result += p.sign_;
    }
    else
        result += "Не подключен";

    return result;
}

std::string Device_Item::get_name() const
{
    const QString dev_item_sql =
            "SELECT IF(di.name IS NULL OR di.name = '', dit.title, di.name) as title FROM das_device_item di "
            "LEFT JOIN das_device_item_type dit ON dit.id = di.type_id "
            "WHERE di."
            + parameters().scheme_->ids_to_sql() +
            " AND di.id = "
            + QString::number(id());

    Base& db = Base::get_thread_local_instance();
    auto q = db.exec(dev_item_sql);
    if (q.next())
        return q.value(0).toString().toStdString();

    return "Unknown";
}

Device_Item::Value_Pair Device_Item::get_value_pair() const
{
    const QString dev_item_sql =
            "SELECT dv.value, s.name FROM das_device_item di "
            "LEFT JOIN das_device_item_type dit ON dit.id = di.type_id "
            "LEFT JOIN das_device_item_value dv ON dv.item_id = di.id AND dv.scheme_id = "
            + QString::number(parameters().scheme_->id()) +
            " LEFT JOIN das_sign_type s ON s.id = dit.sign_id "
            "WHERE di."
            + parameters().scheme_->ids_to_sql() +
            " AND di.id = "
            + QString::number(id());

    Base& db = Base::get_thread_local_instance();
    auto q = db.exec(dev_item_sql);
    if (q.next())
    {
        QVariant value = DB::Device_Item_Value::variant_from_string(q.value(0));
        return Value_Pair{value, q.value(1).toString().toStdString()};
    }
    return {};
}

void Device_Item::parse(const picojson::object &data)
{
    Named_Data_Item::parse(data);

    auto it = data.find("norm");
    if (it != data.cend())
    {
        const std::string expression_string = it->second.get<std::string>();
        normalizer_ = std::make_shared<Device_Item_Value_Normalizer>(expression_string);
    }
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
