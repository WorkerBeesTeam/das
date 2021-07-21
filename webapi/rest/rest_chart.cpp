
#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QSqlError>

#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include <plus/das/database_delete_info.h>

#include "json_helper.h"
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
	mux.handle(chart_url + "{chart_id:[0-9]+}/")
			.del([this](served::response& res, const served::request& req) { remove(res, req); });
	mux.handle(chart_url)
			.get([this](served::response& res, const served::request& req) { get_list(res, req); })
			.put([this](served::response& res, const served::request& req) { upsert(res, req); });
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

picojson::array& get_axis_datasets(picojson::array& axes, const std::string& axis_id, picojson::object& item_axis)
{
	for (picojson::value& val: axes)
		if (val.get("id").get<std::string>() == axis_id)
			return val.get("datasets").get<picojson::array>();

	item_axis.emplace("datasets", picojson::array{});
	axes.emplace_back(std::move(item_axis));
	return axes.back().get("datasets").get<picojson::array>();
}

void parse_dataset_to_axis(const DB::Chart_Item& item, picojson::array& axes, picojson::array& items)
{
	picojson::value extra;
	picojson::parse(extra, item.extra().toStdString());

	picojson::object dataset;
	dataset.emplace("isParam", !item.item_id());
	dataset.emplace("item_id", item.item_id() ? picojson::value(static_cast<int64_t>(item.item_id())) : picojson::value{});
	dataset.emplace("param_id", item.param_id() ? picojson::value(static_cast<int64_t>(item.param_id())) : picojson::value{});

	try {
		picojson::object& item_axis = extra.get("axis").get<picojson::object>();

		std::string axis_id = item_axis["id"].get<std::string>();
		if (axis_id.empty())
			throw 1;

		picojson::array& datasets = get_axis_datasets(axes, axis_id, item_axis);
		extra.get<picojson::object>().erase("axis");
		dataset.emplace("extra", std::move(extra));

		datasets.emplace_back(std::move(dataset));
	}
	catch (...) {
		dataset.emplace("extra", std::move(extra));
		items.emplace_back(std::move(dataset));
	}
}

picojson::object get_chart(const QString& scheme_ids, const DB::Chart& chart)
{
	picojson::array items;
	picojson::array axes;

	Base& db = Base::get_thread_local_instance();
	const Table item_table = db_table<DB::Chart_Item>();
	const QString where = scheme_ids + " AND chart_id = " + QString::number(chart.id());
	QSqlQuery item_q = db.select(item_table, "WHERE " + where);
	while (item_q.next())
	{
		try {
			DB::Chart_Item item = db_build<DB::Chart_Item>(item_q);
			parse_dataset_to_axis(item, axes, items);
		} catch (...) {}
	}

	picojson::object obj;
	obj.emplace("id", static_cast<int64_t>(chart.id()));
	obj.emplace("name", chart.name().toStdString());
	if (axes.size() >= items.size())
		obj.emplace("axes", std::move(axes));
	else
		obj.emplace("items", std::move(items));
	return obj;
}

picojson::array get_all_charts(const QString& scheme_ids)
{
	Base& db = Base::get_thread_local_instance();

	const Table table = db_table<DB::Chart>();

	auto q = db.select(table, "WHERE " + scheme_ids);
	if (!q.isActive())
		throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());

	picojson::array json;

	while (q.next())
	{
		const DB::Chart chart = db_build<DB::Chart>(q);
		json.emplace_back(get_chart(scheme_ids, chart));
	}

	return json;
}

void Chart::get_list(served::response& res, const served::request& req)
{
	Auth_Middleware::check_permission("view_chart");
	const Scheme_Info scheme = Scheme::get_info(req);

	picojson::array json = get_all_charts(scheme.ids_to_sql());

	res.set_header("Content-Type", "application/json");
	res << picojson::value{std::move(json)}.serialize();
}

std::vector<DB::Chart_Item> parse_axes(uint32_t chart_id, uint32_t scheme_id, const picojson::array& axes)
{
	auto get_uint32_or_zero = [](const picojson::value& val, const std::string &key) -> uint32_t
	{
		const picojson::value& key_val = val.get(key);
		return key_val.is<picojson::null>() ? 0 : static_cast<uint32_t>(key_val.get<int64_t>());
	};

	std::vector<DB::Chart_Item> items;
	for (const picojson::value& val: axes)
	{
		const picojson::array& datasets = val.get("datasets").get<picojson::array>();
		if (!datasets.empty() && !val.get("id").get<std::string>().empty())
		{
			picojson::value axis = val;
			axis.get<picojson::object>().erase("datasets");

			for (const picojson::value& dataset: datasets)
			{
				picojson::object extra = dataset.get("extra").get<picojson::object>();
				extra.emplace("axis", axis);

				items.emplace_back(0, chart_id, QString::fromStdString(picojson::value{std::move(extra)}.serialize()),
								   get_uint32_or_zero(dataset, "item_id"),
								   get_uint32_or_zero(dataset, "param_id"), scheme_id);
			}
		}
	}

	return items;
}

void Chart::upsert(served::response &res, const served::request &req)
{
    picojson::object chart_obj = Helper::parse_object(req.body());

	const Scheme_Info scheme = Scheme::get_info(req);

	DB::Chart chart{chart_obj.at("id").get<int64_t>(), QString::fromStdString(chart_obj.at("name").get<std::string>()), scheme.id()};
	if (chart.name().isEmpty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Name can't be empty");

	Auth_Middleware::check_permission(chart.id() ? "change_chart" : "add_chart");

    if (!scheme.extending_scheme_ids().empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Can't change custom charts for child scheme");

    Base& db = Base::get_thread_local_instance();

    const Table table = db_table<DB::Chart>();

	if (chart.id())
    {
		const QString where = scheme.ids_to_sql() + " AND id = " + QString::number(chart.id());
        auto q = db.select(table, "WHERE " + where);
        if (!q.isActive() || !q.next())
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());

		const DB::Chart old_chart = db_build<DB::Chart>(q);

		if (chart.name() != old_chart.name())
        {
			q = db.update(table, {chart.name()}, where, {DB::Chart::COL_name});
            if (!q.isActive())
                throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
        }
    }
    else
	{
        QVariant new_id;
        bool res = db.insert(table, DB::Chart::to_variantlist(chart), &new_id);
		chart.set_id(new_id.toUInt());

		if (!res || chart.id() == 0)
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, "Insert chart failed");
    }

	std::vector<DB::Chart_Item> chart_item_vect = parse_axes(chart.id(), scheme.id(), chart_obj.at("axes").get<picojson::array>()),
			del_vect, upd_vect;

    const Table item_table = db_table<DB::Chart_Item>();
	const QString where = scheme.ids_to_sql() + " AND chart_id = " + QString::number(chart.id());

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
            del_vect.push_back(chart_item);
        else
        {
            if (chart_item.extra() != it->extra())
				upd_vect.push_back(std::move(*it));
            chart_item_vect.erase(it);
        }
	}

	q.clear();

	auto get_item_where = [](const DB::Chart_Item& item) -> QString
	{
		if (item.item_id())
			return " AND item_id = " + QString::number(item.item_id());
		return " AND param_id = " + QString::number(item.param_id());
	};

    for (const DB::Chart_Item& item: del_vect)
    {
        const auto q = db.del(item_table.name(), where + get_item_where(item));
        if (!q.isActive())
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
    }

    for (const DB::Chart_Item& item: upd_vect)
    {
        const auto q = db.update(item_table, {item.extra()}, where + get_item_where(item), {DB::Chart_Item::COL_extra});
        if (!q.isActive())
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
    }

    for (const DB::Chart_Item& item: chart_item_vect)
    {
        bool res = db.insert(item_table, DB::Chart_Item::to_variantlist(item));
        if (!res)
			throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
    }

    res.set_header("Content-Type", "application/json");
	res << picojson::value{get_chart(scheme.ids_to_sql(), chart)}.serialize();
}

} // namespace Rest
} // namespace Das
