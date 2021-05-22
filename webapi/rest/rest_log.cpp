
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

#include <fmt/core.h>

#include <served/status.hpp>
#include <served/request_error.hpp>

#include <Das/log/log_pack.h>

#include "json_helper.h"
#include "auth_middleware.h"
#include "rest_scheme.h"
#include "rest_log.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

Log::Log(served::multiplexer &mux, const std::string &scheme_path)
{
    const std::string log_url = scheme_path + "/log/";

    add_getter_handler<DB::Log_Event_Item>(mux, log_url, "event");
    add_getter_handler<DB::Log_Mode_Item>(mux, log_url, "mode");
    add_getter_handler<DB::Log_Param_Item>(mux, log_url, "param");
    add_getter_handler<DB::Log_Status_Item>(mux, log_url, "status");
    add_getter_handler<DB::Log_Value_Item>(mux, log_url, "value");
    // TODO: реализация filter case_sensitive
    // TODO: реализация dig_id dig_param_id item_id

    /*
    LOG EVENT
    SELECT l.timestamp_msecs, l.user_id, NULL AS group_id, NULL as item_id, type_id, NULL AS param_id, l.category, l.text, NULL AS args FROM das_log_event l

    LOG MODE
    SELECT l.timestamp_msecs, l.user_id, l.group_id, NULL as item_id, 10 AS type_id, NULL AS param_id, 'mode' as category, mt.title as text, NULL AS args FROM das_log_mode l
    LEFT JOIN das_dig_mode_type mt ON mt.id = l.mode_id

    LOG PARAM
    SELECT l.timestamp_msecs, l.user_id, p.group_id, NULL as item_id, 20 AS type_id, p.param_id, 'param' as category, value as text, NULL AS args FROM das_log_param l
    LEFT JOIN das_dig_param p ON p.id = l.group_param_id

    LOG STATUS
    SELECT l.timestamp_msecs, l.user_id, l.group_id, NULL as item_id, 30 AS type_id, NULL AS param_id, 'status' as category, CONCAT(IF (l.direction = 2, '🆙 ', IF (sc.name = 'Error', '🚨 ', IF (sc.name = 'Warn', '⚠️ ', IF (sc.name = 'Ok', '✅ ', '')))),  CAST(st.text AS CHAR CHARACTER SET utf16)) as text, l.args FROM das_log_status l
    LEFT JOIN das_dig_status_type st ON st.id = l.status_id
    LEFT JOIN das_dig_status_category sc ON sc.id = st.category_id

    LOG VALUE
    SELECT l.timestamp_msecs, l.user_id, NULL as group_id, l.item_id, 40 AS type_id, NULL AS param_id, 'item' as category, CONCAT(l.value, IF (l.raw_value IS NULL, '', CONCAT(' (', l.raw_value, ')'))) as text, NULL AS args FROM das_log_value l




    (SELECT l.timestamp_msecs, l.user_id, NULL AS group_id, NULL as item_id, type_id, NULL AS param_id, l.category, l.text, NULL AS args FROM das_log_event l)
    UNION
    (SELECT l.timestamp_msecs, l.user_id, l.group_id, NULL as item_id, 10 AS type_id, NULL AS param_id, 'mode' as category, mt.title as text, NULL AS args FROM das_log_mode l
    LEFT JOIN das_dig_mode_type mt ON mt.id = l.mode_id)
    UNION
    (SELECT l.timestamp_msecs, l.user_id, p.group_id, NULL as item_id, 20 AS type_id, p.param_id, 'param' as category, value as text, NULL AS args FROM das_log_param l
    LEFT JOIN das_dig_param p ON p.id = l.group_param_id)
    UNION
    (SELECT l.timestamp_msecs, l.user_id, l.group_id, NULL as item_id, 30 AS type_id, NULL AS param_id, 'status' as category, CONCAT(IF (l.direction = 2, '🆙 ', IF (sc.name = 'Error', '🚨 ', IF (sc.name = 'Warn', '⚠️ ', IF (sc.name = 'Ok', '✅ ', '')))), CAST(st.text AS CHAR CHARACTER SET utf16)) as text, l.args FROM das_log_status l
    LEFT JOIN das_dig_status_type st ON st.id = l.status_id
    LEFT JOIN das_dig_status_category sc ON sc.id = st.category_id)
    UNION
    (SELECT l.timestamp_msecs, l.user_id, NULL as group_id, l.item_id, 40 AS type_id, NULL AS param_id, 'item' as category, CONCAT(l.value, IF (l.raw_value IS NULL, '', CONCAT(' (', l.raw_value, ')'))) as text, NULL AS args FROM das_log_value l)
    ORDER BY timestamp_msecs DESC
    limit 30
    */


    /*
    SELECT l.timestamp_msecs, l.user_id, l.group_id, CONCAT(IF (l.direction = 2, '🆙 ', IF (sc.name = 'Error', '🚨 ', IF (sc.name = 'Warn', '⚠️ ', IF (sc.name = 'Ok', '✅ ', '')))),  CAST(st.text AS CHAR CHARACTER SET utf16)) as text, l.args, l.scheme_id
    FROM das_log_status l
    LEFT JOIN das_dig_status_type st ON st.id = l.status_id
    LEFT JOIN das_dig_status_category sc ON sc.id = st.category_id
    ORDER BY l.timestamp_msecs DESC
    LIMIT 5
    */






    /* DIG MODE
    SELECT l.timestamp_msecs, l.user_id, CONCAT('[mode] ', s.name, ' -> ', IF (g.title IS NULL OR g.title = '', gt.title, g.title)) as category, mt.title as text, 7 as type_id, l.scheme_id FROM `das_log_mode` l
    LEFT JOIN das_dig_mode_type mt ON mt.id = l.mode_id
    LEFT JOIN das_device_item_group g ON g.id = l.group_id
    LEFT JOIN das_section s ON s.id = g.section_id
    LEFT JOIN das_dig_type gt ON gt.id = g.type_id
    ORDER BY `timestamp_msecs`  DESC
    LIMIT 5
    */

    /* DIG PARAM
    SELECT l.timestamp_msecs, l.user_id, CONCAT('[param] ', s.name, ' -> ', IF (g.title IS NULL OR g.title = '', gt.title, g.title), ' -> ', get_dig_param_title(pt.id)) as categiory, l.value as text, 8 as type_id, l.scheme_id FROM `das_log_param` l
    LEFT JOIN das_dig_param p ON p.id = l.group_param_id
    LEFT JOIN das_device_item_group g ON g.id = p.group_id
    LEFT JOIN das_section s ON s.id = g.section_id
    LEFT JOIN das_dig_type gt ON gt.id = g.type_id
    LEFT JOIN das_dig_param_type pt ON pt.id = p.param_id
    ORDER BY `timestamp_msecs`  DESC
    LIMIT 5
    */

    /*
     *
    DELIMITER $$
    CREATE DEFINER=`kirill_db`@`localhost` PROCEDURE `getDIGParamTitle`(IN `dig_param_id` INT, OUT `title` TEXT CHARSET utf8)
        NO SQL
        SQL SECURITY INVOKER
    BEGIN
    DECLARE p_id INT;
    DECLARE res TEXT;
    SET max_sp_recursion_depth=50;

    SELECT pt.`title`, pt.`parent_id` INTO title, p_id FROM das_dig_param_type pt WHERE pt.id = dig_param_id;

    IF p_id IS NOT NULL THEN
        CALL getDIGParamTitle(p_id, res);
        SELECT CONCAT(res, ' -> ', title) INTO title;
    END IF;
    END$$
    DELIMITER ;

    DELIMITER $$
    CREATE DEFINER=`kirill_db`@`localhost` FUNCTION `get_dig_param_title`(`dig_param_id` INT) RETURNS text CHARSET utf8
        NO SQL
    BEGIN
    DECLARE res TEXT;

    CALL getDIGParamTitle(dig_param_id, res);
    return res;

    END$$
    DELIMITER ;
    */
}

/*static*/ Log_Time_Range Log::get_time_range(const std::string &from_str, const std::string &to_str)
{
    Log_Time_Range time_range{ std::stoll(from_str), std::stoll(to_str) };
    if (/*time_range._from == 0 || */time_range._to == 0 || time_range._from > time_range._to)
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Invalid date range");
    return time_range;
}

/*static*/ Log_Time_Range Log::parse_time_range(const served::request &req)
{
    return get_time_range(req.query["ts_from"], req.query["ts_to"]);
}

/*static*/ std::vector<std::string> Log::parse_data_in(const std::string &param)
{
    std::vector<std::string> data_string_list, res;
    boost::split(data_string_list, param, [](char c) { return c == ','; });

    for (const std::string& item: data_string_list)
    {
        const uint32_t item_id = std::stoul(item);
        if (item_id)
            res.push_back(std::to_string(item_id));
    }

    if (res.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Invalid data items");
    return res;
}

template<typename T>
void Log::add_getter_handler(served::multiplexer &mux, const std::string &base_path, const std::string& name)
{
    mux.handle(base_path + name + '/').get([this, name](served::response& res, const served::request& req)
    {
        log_getter<T>(res, req, name);
    });
}

inline std::string parse_and_join(const std::string& ids)
{
    const std::vector<std::string> data = Log::parse_data_in(ids);
    return boost::join(data, ",");
}

template<typename T>
std::pair<std::string, std::string> parse_ids_filter(const Log_Query_Param& config)
{
    return { "group_id", parse_and_join(config._dig_id) };
}

template<> std::pair<std::string, std::string> parse_ids_filter<DB::Log_Event_Item>(const Log_Query_Param&) { return {}; }
template<> std::pair<std::string, std::string> parse_ids_filter<DB::Log_Param_Item>(const Log_Query_Param& config)
{
    return { "group_param_id", parse_and_join(config._dig_param_id) };
}
template<> std::pair<std::string, std::string> parse_ids_filter<DB::Log_Value_Item>(const Log_Query_Param& config)
{
    return { "item_id", parse_and_join(config._item_id) };
}


template<typename T> std::vector<std::string> get_filtred_fields() { return {}; }
template<> std::vector<std::string> get_filtred_fields<DB::Log_Event_Item>() { return {"text", "category"}; }
template<> std::vector<std::string> get_filtred_fields<DB::Log_Param_Item>() { return {"value"}; }
template<> std::vector<std::string> get_filtred_fields<DB::Log_Value_Item>() { return {"value", "raw_value"}; }

template<typename T>
void Log::log_getter(served::response &res, const served::request &req, const std::string& name)
{
    Log_Query_Param config = parse_params(req);

    Auth_Middleware::check_permission("view_log_" + name);

    Table table = db_table<T>();
    QString sql = "WHERE " + table.field_names().at(T::COL_scheme_id) + "=? AND ";

    const QString ts_field_name = table.field_names().at(T::COL_timestamp_msecs);
    sql += ts_field_name + ">=? AND " + ts_field_name + "<=?";

    QVariantList values{config._scheme.id(),
                static_cast<qlonglong>(config._time_range._from),
                static_cast<qlonglong>(config._time_range._to)};

    auto [id_field_name, ids] = parse_ids_filter<T>(config);
    if (!id_field_name.empty() && !ids.empty())
        sql += " AND " + QString::fromStdString(id_field_name) + " IN (" + QString::fromStdString(ids) + ')';

    if (!config._filter.empty())
    {
        std::string filter_str;
        const std::vector<std::string> filtred_fields = get_filtred_fields<T>();
        for (const std::string& field: filtred_fields)
        {
            if (!filter_str.empty())
                filter_str += " OR ";
            if (config._case_sensitive)
                filter_str += field + " LIKE ?";
            else
                filter_str += fmt::format("LOWER({}) LIKE LOWER(?)", field);

//            filter_str += " ESCAPE '@'";
            values.push_back('%' + QString::fromStdString(config._filter) + '%' );
//                             .replace('@', "@@").replace('_', "@_").replace('@', "@@") + '%');
        }

        if (!filter_str.empty())
            sql += " AND (" + QString::fromStdString(filter_str) + ')';
    }

    sql += " ORDER BY " + ts_field_name + " DESC";

    if (config._limit == 0 || config._limit > 1000)
        config._limit = 1000;
    sql += " LIMIT " + QString::number(config._limit);

    Base& db = Base::get_thread_local_instance();
    QSqlQuery query = db.select(table, sql, values);

    if (!query.isActive())
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());

    picojson::array json;
    while (query.next())
        json.emplace_back(gen_json_object<T>(query, table.field_names()));

    res.set_header("Content-Type", "application/json");
    res << picojson::value{std::move(json)}.serialize();
}

Log_Query_Param Log::parse_params(const served::request &req)
{
    uint32_t limit = 1000;
    const std::string limit_str = req.query["limit"];
    if (!limit_str.empty())
        limit = std::stoul(limit_str);

    const std::string case_sensitive_str = req.query["case_sensitive"];

    Log_Query_Param params
    {
        std::move(limit),
        Scheme::get_info(req),
        Log::parse_time_range(req),

        !(!case_sensitive_str.empty() && (case_sensitive_str == "0" || case_sensitive_str == "false")),
        req.query["filter"],

        req.query["dig_id"], req.query["dig_param_id"], req.query["item_id"]
    };

    return params;
}

} // namespace Rest
} // namespace Das
