#include <string>
#include <algorithm>

#include <boost/algorithm/string.hpp>

#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QJsonValue>

#include <Das/log/log_value_item.h>

#include "json_helper.h"
#include "auth_middleware.h"
#include "rest_scheme.h"
#include "rest_chart_value.h"

namespace Das {
namespace Rest {

using namespace std;
using namespace chrono_literals;
using namespace Helpz::DB;

Chart_Value::Chart_Value() :
    _db(Base::get_thread_local_instance())
{
}

/*static*/ Chart_Value_Config &Chart_Value::config()
{
    static Chart_Value_Config conf;
    return conf;
}

std::string Chart_Value::operator()(const served::request &req)
{
    parse_params(req);

    Auth_Middleware::check_permission(permission_name());

    int64_t count = fill_datamap();
    if (_range_close_to_now)
    {
        _time_range._from = _time_range._to;
        _time_range._to = DB::Log_Base_Item::current_timestamp();
        count += fill_datamap();
    }

    picojson::array results;
    fill_results(results);

    picojson::object json;
    json.emplace("count", count);
    json.emplace("results", std::move(results));
    return picojson::value(std::move(json)).serialize();
}

std::string Chart_Value::permission_name() const
{
    return "view_log_value";
}

QString Chart_Value::get_table_name() const
{
    const uint32_t period_sec = ceil((_time_range._to - _time_range._from) / 1000.);
    if (period_sec > config()._day_ratio_sec)
        return Log_Value_Item::table_name() + "_day";
    else if (period_sec > config()._hour_ratio_sec)
        return Log_Value_Item::table_name() + "_hour";
    else if (period_sec > config()._minute_ratio_sec)
        return Log_Value_Item::table_name() + "_minute";
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
    _time_range = Log::parse_time_range(req);
    _data_in_list = Log::parse_data_in(req.query["data"]);

    _scheme_where = get_scheme_where(req);
    _where = get_where();

    parse_limits(req.query["offset"], req.query["limit"]);

    qint64 now = DB::Log_Base_Item::current_timestamp();
    _range_in_past = _time_range._to < now;

    now -= config()._close_to_now_sec * 1000;
    _range_close_to_now = _time_range._to >= now
                          && (_time_range._to - _time_range._from) < config()._hour_ratio_sec;
    if (_range_close_to_now)
        _time_range._to = now;

    string bo = req.query["bounds_only"];
    _bounds_only = !bo.empty() && (bo == "1" || bo == "true");
}

QString Chart_Value::get_where() const
{
    return "WHERE " + _scheme_where + " AND " + get_data_in_where() + " AND " + get_time_range_where();
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

QString Chart_Value::get_data_in_where() const
{
    return get_field_name(FT_ITEM_ID) + " IN (" + QString::fromStdString(boost::join(_data_in_list, ",")) + ')';
}

void Chart_Value::parse_limits(const std::string &offset_str, const std::string &limit_str)
{
    _offset = stoa_or(offset_str);
    _limit = stoa_or(limit_str, 1000000UL);
    if (_limit == 0 || _limit > 1000000)
        _limit = 1000000;
}

QString Chart_Value::get_limit_suffix(uint32_t offset, uint32_t limit) const
{
    return "LIMIT " + QString::number(offset) + ',' + QString::number(limit);
}

int64_t Chart_Value::fill_datamap()
{
    int64_t count = 0, timestamp;
    uint32_t item_id;

    const QString sql = _bounds_only ? get_bounds_sql() : get_full_sql();
    QSqlQuery q = _db.exec(sql);
    auto it = _data_map.end();

    while (q.next())
    {
        timestamp = q.value(FT_TIME).toLongLong();
        item_id = q.value(FT_ITEM_ID).toUInt();

        if (it == _data_map.end() || it->first != item_id)
        {
            it = _data_map.find(item_id);
            if (it == _data_map.cend())
                it = _data_map.emplace(item_id, std::map<int64_t, picojson::object>{}).first;
        }

        if (_bounds_only)
            timestamp = timestamp <= _time_range._from ? _time_range._from : _time_range._to;

        it->second.emplace(timestamp, get_data_item(q, timestamp));
        ++count;
    }

    return count;
}

QString Chart_Value::get_full_sql() const
{
    return get_base_sql() + ' ' + _where + ' ' + get_limit_suffix(_offset, _limit);
    //            + ';' + get_base_sql("COUNT(*)") + ' ' + _where
}

QString Chart_Value::get_bounds_sql() const
{
    QStringList one_point_sql_list;
    for (const std::string& item_id_s: _data_in_list)
    {
        const QString item_id = QString::fromStdString(item_id_s);
        one_point_sql_list.push_back(get_one_point_sql(_time_range._from, item_id, true));
        if (_range_in_past)
            one_point_sql_list.push_back(get_one_point_sql(_time_range._to, item_id, false));
    }
    return '(' + one_point_sql_list.join(") UNION (") + ')';
}

QString Chart_Value::get_one_point_sql(int64_t timestamp, const QString& item_id, bool is_before_range_point) const
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
    sql += item_id;
    sql += " ORDER BY ";
    sql += time_field_name;
    if (is_before_range_point)
        sql += " DESC";
    sql += ' ';
    sql += limit_suffix;
    return sql;
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
    try
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
    }
    catch (const std::exception&) {}
    catch (...) {}

    return picojson::value(value.toString().toStdString());
}

void Chart_Value::fill_object(picojson::object &obj, const QSqlQuery &q) const
{
    const int64_t user_id = q.value(FT_USER_ID).toLongLong();
    if (user_id)
        obj.emplace("user_id", user_id);

    const QVariant value = Device_Item_Value::variant_from_string(q.value(FT_VALUE));
    obj.emplace("value", variant_to_json(value));

    fill_additional_fields(obj, q, value);
}

void Chart_Value::fill_results(picojson::array &results) const
{
//    const qint64 now = DB::Log_Base_Item::current_timestamp();

    for (const auto& it: _data_map)
    {
        picojson::array data_arr;
        for (auto&& data_it: it.second)
            data_arr.push_back(picojson::value(std::move(data_it.second)));

        picojson::object obj;
        obj.emplace("item_id", static_cast<int64_t>(it.first));
        obj.emplace("data", std::move(data_arr));

        results.push_back(picojson::value(std::move(obj)));
    }
}

} // namespace Rest
} // namespace Das
