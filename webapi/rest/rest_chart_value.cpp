#include <string>

#include <boost/algorithm/string.hpp>

#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QJsonValue>

#include <Helpz/db_base.h>

#include <Das/log/log_value_item.h>

#include "auth_middleware.h"
#include "rest_scheme.h"
#include "rest_chart_value.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

Chart_Value::Chart_Value(served::multiplexer &mux, const std::string &scheme_path)
{
    const std::string url = scheme_path + "/chart_value/";
    mux.handle(url).get([this](served::response& res, const served::request& req) { list(res, req); });
}

void Chart_Value::list(served::response &res, const served::request &req)
{
    const Time_Range time_range = get_time_range(req.query["ts_from"], req.query["ts_to"]);
    const QString scheme_where = get_scheme_where(req);
    const QString where = get_where(req, time_range, scheme_where);

    Auth_Middleware::check_permission("view_log_value");

    uint32_t offset, limit;
    const QString limit_suffix = get_limits(req.query["offset"], req.query["limit"], offset, limit);

    const QString sql = get_base_sql() + ' ' + where + ' ' + limit_suffix;
    std::map<uint32_t, std::map<qint64, picojson::object>> data_map;
    int64_t count = fill_datamap(data_map, sql);

    int64_t count_all = get_count_all(count, offset, limit, where);

    picojson::array results;
    fill_results(results, data_map, time_range, scheme_where);

    picojson::object json;
    json.emplace("count", count);
    json.emplace("count_all", count_all);
    json.emplace("results", std::move(results));
    res << picojson::value(std::move(json)).serialize();
}

QString Chart_Value::get_base_sql(const QString& what) const
{
    QString sql = "SELECT ";
    if (what.isEmpty())
    {
        QStringList column_names = Log_Value_Item::table_column_names();

        const char* delim = ", ";
        sql += column_names.at(Log_Value_Item::COL_timestamp_msecs);
        sql += delim;
        sql += column_names.at(Log_Value_Item::COL_item_id);
        sql += delim;
        sql += column_names.at(Log_Value_Item::COL_user_id);
        sql += delim;
        sql += column_names.at(Log_Value_Item::COL_raw_value);
        sql += delim;
        sql += column_names.at(Log_Value_Item::COL_value);
    }
    else
        sql += what;

    sql += " FROM ";
    sql += Log_Value_Item::table_name();
    return sql;
}

QString Chart_Value::get_where(const served::request &req, const Time_Range& time_range, const QString& scheme_where) const
{
    const QString time_range_where = get_time_range_where(time_range);
    const QString data_in_string = get_data_in_string(req.query["data"]);

    QString where = "WHERE ";
    where += time_range_where;
    where += " AND ";
    where += scheme_where;
    where += " AND ";
    where += data_in_string;
    return where;
}

Chart_Value::Time_Range Chart_Value::get_time_range(const std::string &from_str, const std::string &to_str) const
{
    Time_Range time_range{ std::stoll(from_str), std::stoll(to_str) };
    if (time_range._from == 0 || time_range._to == 0 || time_range._from > time_range._to)
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Invalid date range");
    return time_range;
}

QString Chart_Value::get_time_field_name() const
{
    return Log_Value_Item::table_column_names().at(Log_Value_Item::COL_timestamp_msecs);
}

QString Chart_Value::get_time_range_where(const Time_Range& time_range) const
{
    const QString field_name = get_time_field_name();
    return field_name + " >= " + QString::number(time_range._from) + " AND " + field_name + " <= " + QString::number(time_range._to);
}

QString Chart_Value::get_scheme_field_name() const
{
    return Log_Value_Item::table_column_names().at(Log_Value_Item::COL_scheme_id);
}

QString Chart_Value::get_scheme_where(const served::request& req) const
{
    const Scheme_Info scheme = Scheme::get_info(req);

    QString sql = get_scheme_field_name();
    sql += " = ";
    sql += QString::number(scheme.id());
    return sql;
}

QString Chart_Value::get_data_field_name() const
{
    return Log_Value_Item::table_column_names().at(Log_Value_Item::COL_item_id);
}

QString Chart_Value::get_data_in_string(const std::string &param) const
{
    std::vector<std::string> data_string_list;
    boost::split(data_string_list, param, [](char c) { return c == ','; });

    QString data_in_string;
    for (const std::string& item: data_string_list)
    {
        const uint32_t item_id = std::stoul(item);
        if (item_id)
        {
            if (!data_in_string.isEmpty())
                data_in_string += ',';
            data_in_string += QString::number(item_id);
        }
    }

    if (data_in_string.isEmpty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Invalid data items");

    return get_data_field_name() + " IN (" + data_in_string + ')';
}

template<typename T>
T stoa_or_0(T(*func)(const std::string&, size_t*, int), const std::string& text, T default_value = static_cast<T>(0))
{
    try {
        return func(text, 0, 10);
    } catch(...) {}
    return default_value;
}

QString Chart_Value::get_limits(const std::string &offset_str, const std::string &limit_str, uint32_t &offset, uint32_t &limit) const
{
    offset = stoa_or_0(std::stoul, offset_str);
    limit = stoa_or_0(std::stoul, limit_str, 1000000UL);
    if (limit == 0 || limit > 1000000)
        limit = 1000000;

    return get_limit_suffix(offset, limit);
}

QString Chart_Value::get_limit_suffix(uint32_t offset, uint32_t limit) const
{
    return "LIMIT " + QString::number(offset) + ',' + QString::number(limit);
}

int64_t Chart_Value::get_count_all(int64_t count, uint32_t offset, uint32_t limit, const QString &where)
{
    if (count < limit)
    {
        count += offset;
    }
    else
    {
        const QString sql = get_base_sql("COUNT(*)") + ' ' + where;
        Base& db = Base::get_thread_local_instance();
        QSqlQuery q = db.exec(sql);
        if (q.next())
            count = q.value(0).toLongLong();
    }

    return count;
}

int64_t Chart_Value::fill_datamap(std::map<uint32_t, std::map<qint64, picojson::object> > &data_map, const QString& sql) const
{
    int64_t count = 0;

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql);
    while (q.next())
    {
        qint64 timestamp = q.value(0).toLongLong(); // 0 = COL_timestamp_msecs
        const uint32_t item_id = q.value(1).toUInt();

        std::map<qint64, picojson::object>& item_data = data_map[item_id];
        item_data.emplace(timestamp, get_data_item(q, timestamp));

        ++count;
    }

    return count;
}

picojson::object Chart_Value::get_data_item(const QSqlQuery& query, int64_t timestamp) const
{
    picojson::object obj;
    fill_object(obj, query);
    obj.emplace("time", static_cast<int64_t>(timestamp));
    return obj;
}

picojson::value variant_to_json(const QVariant& value)
{
    const QJsonValue data = QJsonValue::fromVariant(value);
    switch (data.type())
    {
    case QJsonValue::Null:
    case QJsonValue::Undefined:
        return picojson::value();

    case QJsonValue::Bool:
        return picojson::value(data.toBool());

    case QJsonValue::Double:
        return picojson::value(data.toDouble());

    case QJsonValue::String:
        return picojson::value(data.toString().toStdString());
    default:
        break;
    }

    return picojson::value(value.toString().toStdString());
}

void Chart_Value::fill_object(picojson::object &obj, const QSqlQuery &q) const
{
    const int64_t user_id = q.value(2).toLongLong();
    if (user_id)
        obj.emplace("user_id", user_id);

    const QVariant raw_value = Log_Value_Item::variant_from_string(q.value(3));
    const QVariant value     = Log_Value_Item::variant_from_string(q.value(4));

    obj.emplace("value", variant_to_json(value));
    if (value != raw_value)
        obj.emplace("raw_value", variant_to_json(raw_value));
}

void Chart_Value::fill_results(picojson::array &results, const std::map<uint32_t, std::map<qint64, picojson::object> > &data_map,
                               const Chart_Value::Time_Range& time_range, const QString& scheme_where) const
{
//    const qint64 now = DB::Log_Base_Item::current_timestamp();
    const bool range_in_past = time_range._to < DB::Log_Base_Item::current_timestamp();

    for (const auto& it: data_map)
    {
        picojson::array data_arr;

        if (it.second.empty() || it.second.cbegin()->first > time_range._from)
        {
            picojson::object point = get_one_point(time_range._from, scheme_where, it.first, true);
            if (!point.empty())
                data_arr.push_back(picojson::value(std::move(point)));
        }

        for (auto&& data_it: it.second)
            data_arr.push_back(picojson::value(std::move(data_it.second)));

        if (range_in_past
            && (it.second.empty() || it.second.crbegin()->first < time_range._to))
        {
            picojson::object point = get_one_point(time_range._to, scheme_where, it.first, false);
            if (!point.empty())
                data_arr.push_back(picojson::value(std::move(point)));
        }

        picojson::object obj;
        obj.emplace("item_id", static_cast<int64_t>(it.first));
        obj.emplace("data", std::move(data_arr));

        results.push_back(picojson::value(std::move(obj)));
    }
}

picojson::object Chart_Value::get_one_point(int64_t timestamp, const QString& scheme_where, uint32_t item_id, bool is_before_range_point) const
{
    static QString limit_suffix = get_limit_suffix(0, 1);

    const QString time_field_name = get_time_field_name();
    QString sql = get_base_sql();
    sql += " WHERE ";
    sql += time_field_name;
    sql += is_before_range_point ? " < " : " > ";
    sql += QString::number(timestamp);
    sql += " AND ";
    sql += scheme_where;
    sql += " AND ";
    sql += get_data_field_name();
    sql += " = ";
    sql += QString::number(item_id);
    sql += " ORDER BY ";
    sql += time_field_name;
    if (is_before_range_point)
        sql += " DESC";
    sql += ' ';
    sql += limit_suffix;

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql);
    if (q.next())
        return get_data_item(q, timestamp);
    return {};
}

} // namespace Rest
} // namespace Das
