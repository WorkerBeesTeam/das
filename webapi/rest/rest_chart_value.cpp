#include <string>

#include <boost/algorithm/string.hpp>

#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QJsonValue>

#include <Das/log/log_value_item.h>

#include "auth_middleware.h"
#include "rest_scheme.h"
#include "rest_chart_value.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

Chart_Value::Chart_Value() :
    _db(Base::get_thread_local_instance())
{
}

std::string Chart_Value::operator()(const served::request &req)
{
    parse_params(req);

    Auth_Middleware::check_permission(permission_name());

    int64_t count = fill_datamap();

    int64_t count_all = get_count_all(count);

    picojson::array results;
    fill_results(results);

    picojson::object json;
    json.emplace("count", count);
    json.emplace("count_all", count_all);
    json.emplace("results", std::move(results));
    return picojson::value(std::move(json)).serialize();
}

std::string Chart_Value::permission_name() const
{
    return "view_log_value";
}

QString Chart_Value::get_table_name() const
{
    return Log_Value_Item::table_name();
}

QString Chart_Value::get_field_name(Chart_Value::Field_Type field_type) const
{
    static QStringList column_names = Log_Value_Item::table_column_names();

    switch (field_type)
    {
    case FT_TIME:       return column_names.at(Log_Value_Item::COL_timestamp_msecs);
    case FT_ITEM_ID:    return column_names.at(Log_Value_Item::COL_item_id);
    case FT_USER_ID:    return column_names.at(Log_Value_Item::COL_user_id);
    case FT_VALUE:      return column_names.at(Log_Value_Item::COL_value);
    case FT_SCHEME_ID:  return column_names.at(Log_Value_Item::COL_scheme_id);
    default:
        break;
    }
    return {};
}

QString Chart_Value::get_additional_field_names() const
{
    return Log_Value_Item::table_column_names().at(Log_Value_Item::COL_raw_value);
}

void Chart_Value::fill_additional_fields(picojson::object &obj, const QSqlQuery &q, const QVariant &value) const
{
    const QVariant raw_value = Device_Item_Value::variant_from_string(q.value(FT_VALUE + 1));

    if (value != raw_value)
        obj.emplace("raw_value", variant_to_json(raw_value));
}

void Chart_Value::parse_params(const served::request &req)
{
    _time_range = get_time_range(req.query["ts_from"], req.query["ts_to"]);
    _scheme_where = get_scheme_where(req);
    _where = get_where(req.query["data"]);

    parse_limits(req.query["offset"], req.query["limit"]);
}

QString Chart_Value::get_where(const std::string &data_in_param) const
{
    const QString time_range_where = get_time_range_where();
    const QString data_in_string = get_data_in_string(data_in_param);

    QString where = "WHERE ";
    where += time_range_where;
    where += " AND ";
    where += _scheme_where;
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

QString Chart_Value::get_time_range_where() const
{
    const QString field_name = get_field_name(FT_TIME);
    return field_name + " >= " + QString::number(_time_range._from) + " AND " + field_name + " <= " + QString::number(_time_range._to);
}

QString Chart_Value::get_scheme_where(const served::request& req) const
{
    const Scheme_Info scheme = Scheme::get_info(req);

    QString sql = get_field_name(FT_SCHEME_ID);
    sql += " = ";
    sql += QString::number(scheme.id());
    return sql;
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

    return get_field_name(FT_ITEM_ID) + " IN (" + data_in_string + ')';
}

template<typename T>
T stoa_or_0(T(*func)(const std::string&, size_t*, int), const std::string& text, T default_value = static_cast<T>(0))
{
    try {
        return func(text, 0, 10);
    } catch(...) {}
    return default_value;
}

void Chart_Value::parse_limits(const std::string &offset_str, const std::string &limit_str)
{
    _offset = stoa_or_0(std::stoul, offset_str);
    _limit = stoa_or_0(std::stoul, limit_str, 1000000UL);
    if (_limit == 0 || _limit > 1000000)
        _limit = 1000000;
}

QString Chart_Value::get_limit_suffix(uint32_t offset, uint32_t limit) const
{
    return "LIMIT " + QString::number(offset) + ',' + QString::number(limit);
}

int64_t Chart_Value::get_count_all(int64_t count)
{
    if (count < _limit)
    {
        count += _offset;
    }
    else
    {
        const QString sql = get_base_sql("COUNT(*)") + ' ' + _where;
        QSqlQuery q = _db.exec(sql);
        if (q.next())
            count = q.value(0).toLongLong();
    }

    return count;
}

int64_t Chart_Value::fill_datamap()
{
    int64_t count = 0;

    const QString sql = get_base_sql() + ' ' + _where + ' ' + get_limit_suffix(_offset, _limit);
    QSqlQuery q = _db.exec(sql);
    while (q.next())
    {
        qint64 timestamp = q.value(FT_TIME).toLongLong();
        const uint32_t item_id = q.value(FT_ITEM_ID).toUInt();

        std::map<qint64, picojson::object>& item_data = _data_map[item_id];
        item_data.emplace(timestamp, get_data_item(q, timestamp));

        ++count;
    }

    return count;
}

QString Chart_Value::get_base_sql(const QString& what) const
{
    QString sql = "SELECT ";
    if (what.isEmpty())
    {
        const char* delim = ", ";
        sql += get_field_name(FT_TIME);
        sql += delim;
        sql += get_field_name(FT_ITEM_ID);
        sql += delim;
        sql += get_field_name(FT_USER_ID);
        sql += delim;
        sql += get_field_name(FT_VALUE);

        const QString additional_fields = get_additional_field_names();
        if (!additional_fields.isEmpty())
        {
            sql += delim;
            sql += additional_fields;
        }
    }
    else
        sql += what;

    sql += " FROM ";
    sql += get_table_name();
    return sql;
}

picojson::object Chart_Value::get_data_item(const QSqlQuery& query, int64_t timestamp) const
{
    picojson::object obj;
    fill_object(obj, query);
    obj.emplace("time", static_cast<int64_t>(timestamp));
    return obj;
}

picojson::value Chart_Value::variant_to_json(const QVariant& value) const
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
    const int64_t user_id = q.value(FT_USER_ID).toLongLong();
    if (user_id)
        obj.emplace("user_id", user_id);

    const QVariant value     = Device_Item_Value::variant_from_string(q.value(FT_VALUE));
    obj.emplace("value", variant_to_json(value));

    fill_additional_fields(obj, q, value);
}

void Chart_Value::fill_results(picojson::array &results) const
{
//    const qint64 now = DB::Log_Base_Item::current_timestamp();
    const bool range_in_past = _time_range._to < DB::Log_Base_Item::current_timestamp();

    for (const auto& it: _data_map)
    {
        picojson::array data_arr;

        if (it.second.empty() || it.second.cbegin()->first > _time_range._from)
        {
            picojson::object point = get_one_point(_time_range._from, it.first, true);
            if (!point.empty())
                data_arr.push_back(picojson::value(std::move(point)));
        }

        for (auto&& data_it: it.second)
            data_arr.push_back(picojson::value(std::move(data_it.second)));

        if (range_in_past
            && (it.second.empty() || it.second.crbegin()->first < _time_range._to))
        {
            picojson::object point = get_one_point(_time_range._to, it.first, false);
            if (!point.empty())
                data_arr.push_back(picojson::value(std::move(point)));
        }

        picojson::object obj;
        obj.emplace("item_id", static_cast<int64_t>(it.first));
        obj.emplace("data", std::move(data_arr));

        results.push_back(picojson::value(std::move(obj)));
    }
}

picojson::object Chart_Value::get_one_point(int64_t timestamp, uint32_t item_id, bool is_before_range_point) const
{
    static QString limit_suffix = get_limit_suffix(0, 1);

    const QString time_field_name = get_field_name(FT_TIME);
    QString sql = get_base_sql();
    sql += " WHERE ";
    sql += time_field_name;
    sql += is_before_range_point ? " < " : " > ";
    sql += QString::number(timestamp);
    sql += " AND ";
    sql += _scheme_where;
    sql += " AND ";
    sql += get_field_name(FT_ITEM_ID);
    sql += " = ";
    sql += QString::number(item_id);
    sql += " ORDER BY ";
    sql += time_field_name;
    if (is_before_range_point)
        sql += " DESC";
    sql += ' ';
    sql += limit_suffix;

    QSqlQuery q = _db.exec(sql);
    if (q.next())
        return get_data_item(q, timestamp);
    return {};
}

} // namespace Rest
} // namespace Das
