
#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QSqlError>

#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include <plus/das/database_delete_info.h>

#include "rest_helper.h"
#include "auth_middleware.h"
#include "rest_scheme.h"
#include "rest_chart.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

Chart::Chart(served::multiplexer &mux, const std::string &scheme_path)
{
    const std::string chart_url = scheme_path + "/chart/";
    mux.handle(chart_url + "{chart_id:[0-9]+}/").del([this](served::response& res, const served::request& req) { remove(res, req); });
    mux.handle(chart_url).put([this](served::response& res, const served::request& req) { upsert(res, req); });
}

void Chart::remove(served::response &res, const served::request &req)
{
    uint32_t chart_id = std::stoul(req.params["chart_id"]);
    if (chart_id == 0)
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Unknown chart");

    Auth_Middleware::check_permission("delete_chart");

    const Scheme_Info scheme = Scheme::get_info(req);

    Base& db = Base::get_thread_local_instance();
    if (!DB::db_delete_rows<DB::Chart>(db, {chart_id}, scheme))
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Delete chart failed");

    res.set_header("Content-Type", "application/json");
    res << "{\"result\": true}\n";
}

void Chart::upsert(served::response &res, const served::request &req)
{
    picojson::object chart_obj = Helper::parse_object(req.body());

    uint32_t chart_id = chart_obj.at("id").get<int64_t>();
    const std::string& chart_name = chart_obj.at("name").get<std::string>();
    const QString new_name = QString::fromStdString(chart_name);

    if (chart_name.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Name can't be empty");

    Auth_Middleware::check_permission(chart_id ? "change_chart" : "add_chart");

    const Scheme_Info scheme = Scheme::get_info(req);

    if (!scheme.extending_scheme_ids().empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Can't change custom charts for child scheme");

    Base& db = Base::get_thread_local_instance();

    const Table table = db_table<DB::Chart>();

    if (chart_id)
    {
        const QString where = scheme.ids_to_sql() + " AND id = " + QString::number(chart_id);
        auto q = db.select(table, "WHERE " + where);
        if (!q.isActive() || !q.next())
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());

        const DB::Chart chart = db_build<DB::Chart>(q);
        if (chart.id() != chart_id)
            throw served::request_error(served::status_4XX::BAD_REQUEST, "Find chart failed");

        if (chart.name() != new_name)
        {
            q = db.update(table, {new_name}, where, {DB::Chart::COL_name});
            if (!q.isActive())
                throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
        }
    }
    else
    {
        const DB::Chart chart{0, new_name, scheme.id()};
        QVariant new_id;
        bool res = db.insert(table, DB::Chart::to_variantlist(chart), &new_id);
        chart_id = new_id.toUInt();
//        chart_obj["id"].set<int64_t>(chart_id);
//        chart_obj["id"].set<double>(5.);

        if (!res || chart_id == 0)
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, "Insert chart failed");
    }

    const picojson::array chart_items = chart_obj.at("items").get<picojson::array>();

    auto get_uint32_or_zero = [](const picojson::value& val, const std::string &key) -> uint32_t
    {
        const picojson::value& key_val = val.get(key);
        if (key_val.is<picojson::null>())
            return 0;
        else
            return static_cast<uint32_t>(key_val.get<int64_t>());
    };

    std::vector<DB::Chart_Item> chart_item_vect, del_vect, upd_vect, not_modified_vect;
    for (const picojson::value& val: chart_items)
        chart_item_vect.push_back(DB::Chart_Item{0, chart_id, QString::fromStdString(val.get("extra").serialize()),
                                                 get_uint32_or_zero(val, "item_id"),
                                                 get_uint32_or_zero(val, "param_id"), scheme.id()});

    const Table item_table = db_table<DB::Chart_Item>();
    const QString where = scheme.ids_to_sql() + " AND chart_id = " + QString::number(chart_id);

    DB::Chart_Item chart_item;
    QSqlQuery q = db.select(item_table, "WHERE " + where);
    while (q.next())
    {
        chart_item = db_build<DB::Chart_Item>(q);
        auto it = std::find_if(chart_item_vect.begin(), chart_item_vect.end(), [&chart_item](const DB::Chart_Item& item)
        {
            return chart_item.item_id() == item.item_id()
                && chart_item.param_id() == item.param_id();
        });

        if (it == chart_item_vect.end())
        {
            del_vect.push_back(chart_item);
        }
        else
        {
            if (chart_item.extra() != it->extra())
                upd_vect.push_back(std::move(*it));
            else
                not_modified_vect.push_back(std::move(*it));
            chart_item_vect.erase(it);
        }
    }

    auto get_item_where = [](const DB::Chart_Item& item) -> QString
    {
        QString item_where = " AND ";
        if (item.item_id())
            item_where += "item_id = " + QString::number(item.item_id());
        else
            item_where += "param_id = " + QString::number(item.param_id());
        return item_where;
    };

    for (const DB::Chart_Item& item: del_vect)
    {
        const auto q = db.del(item_table.name(), where + get_item_where(item));
        if (!q.isActive())
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
    }

    picojson::array new_items;
    auto add_new_item = [&new_items](const DB::Chart_Item& item)
    {
        picojson::value extra_val;
        try {
            picojson::parse(extra_val, item.extra().toStdString());
        } catch (...) {}

        picojson::object j_item;
        j_item["extra"] = extra_val;
        j_item["item_id"] = item.item_id() ? picojson::value(static_cast<int64_t>(item.item_id())) : picojson::value{};
        j_item["param_id"] = item.param_id() ? picojson::value(static_cast<int64_t>(item.param_id())) : picojson::value{};

        new_items.emplace_back(std::move(j_item));
    };

    for (const DB::Chart_Item& item: upd_vect)
    {
        const auto q = db.update(item_table, {item.extra()}, where + get_item_where(item), {DB::Chart_Item::COL_extra});
        if (!q.isActive())
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
        add_new_item(item);
    }

    for (const DB::Chart_Item& item: chart_item_vect)
    {
        bool res = db.insert(item_table, DB::Chart_Item::to_variantlist(item));
        if (!res)
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, "Insert chart item failed");
        add_new_item(item);
    }

    for (const DB::Chart_Item& item: not_modified_vect)
        add_new_item(item);

    chart_obj["items"].set(std::move(new_items));

    const picojson::value val{std::move(chart_obj)};

    res.set_header("Content-Type", "application/json");
    res << val.serialize();
}

} // namespace Rest
} // namespace Das
