#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QSqlError>
#include <QUuid>

#include <fmt/ranges.h>

#include <Helpz/db_base.h>

//#include <Das/section.h>
#include <Das/db/scheme.h>
#include <Das/db/dig_status_type.h>
#include <Das/db/disabled_status.h>
#include <Das/db/help.h>
//#include <Das/db/dig_status.h>
//#include <Das/db/device_item_value.h>
//#include <Das/db/chart.h>

#include <dbus/dbus_interface.h>

#include "json_helper.h"
#include "filter.h"
#include "csrf_middleware.h"
#include "auth_middleware.h"
#include "scheme_copier.h"
#include "rest_chart.h"
#include "rest_chart_data_controller.h"
#include "rest_scheme.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

const char *get_scheme_base() { return "scheme"; }
std::string get_scheme_path_base() { return '/' + std::string(get_scheme_base()) + '/'; }

std::string get_scheme_path()
{
    return get_scheme_path_base() + '{' + get_scheme_base() + "_id:[0-9]+}";
}

Scheme::Scheme(served::multiplexer& mux, DBus::Interface* dbus_iface) :
    dbus_iface_(dbus_iface)
{
    const std::string scheme_path = get_scheme_path();
    chart_ = std::make_shared<Chart>(mux, scheme_path);
    chart_data_ = std::make_shared<Chart_Data_Controller>(mux, scheme_path);

    mux.handle(scheme_path + "/dig_status").get([this](served::response& res, const served::request& req) { get_dig_status(res, req); });
    mux.handle(scheme_path + "/dig_status_type").get([this](served::response& res, const served::request& req) { get_dig_status_type(res, req); });
    mux.handle(scheme_path + "/device_item_value").get([this](served::response& res, const served::request& req) { get_device_item_value(res, req); });
    mux.handle(scheme_path + "/disabled_status")
            .get([this](served::response& res, const served::request& req) { get_disabled_status(res, req); })
            .method(served::method::PATCH, [this](served::response& res, const served::request& req) { del_disabled_status(res, req); })
            .post([this](served::response& res, const served::request& req) { add_disabled_status(res, req); });

    mux.handle(scheme_path + "/help").get([](served::response &res, const served::request &req)
    {
        Auth_Middleware::check_permission("view_help");

        const Scheme_Info scheme = get_info(req);

        const QString sql = "WHERE scheme_id IS NULL OR %1 ORDER BY parent_id";

        res.set_header("Content-Type", "application/json");
        res << gen_json_list<DB::Help>(sql.arg(scheme.ids_to_sql()));
    });

    mux.handle(scheme_path + "/set_name/").post([this](served::response& res, const served::request& req) { set_name(res, req); });
    mux.handle(scheme_path + "/copy/").post([this](served::response& res, const served::request& req) { copy(res, req); });
    mux.handle(scheme_path).get([this](served::response& res, const served::request& req) { get(res, req); });
    mux.handle(get_scheme_path_base())
            .get([this](served::response& res, const served::request& req) { get_list(res, req); })
            .post([this](served::response& res, const served::request& req) { create(res, req); });
    // throw served::request_error(served::status_4XX::NOT_FOUND, "Scheme not found");
}

/*static*/ Scheme_Info Scheme::get_info(const served::request& req)
{
    const std::string url_path = req.url().path();
    const std::string scheme_path = get_scheme_path_base();
    // Check url_path is starts with /scheme/ and is have more text like /scheme/10/
    if (url_path.size() > scheme_path.size()
        && strncmp(scheme_path.c_str(), url_path.c_str(), scheme_path.size()) == 0)
    {
        const std::string param_name = get_scheme_base() + std::string("_id");
        const std::string param_data = req.params[param_name];
        if (!param_data.empty())
        {
            uint32_t scheme_id = std::atoi(param_data.c_str());
            return get_info(scheme_id);
        }
    }

    throw served::request_error(served::status_4XX::NOT_FOUND, "Scheme not found");
    return {};
}

/*static*/ Scheme_Info Scheme::get_info(uint32_t scheme_id)
{
    uint32_t user_id = Auth_Middleware::get_thread_local_user().id_;
    if (user_id != 0 && scheme_id != 0)
    {
        const QString sql =
                "SELECT s.id, s.parent_id FROM das_scheme s "
                "LEFT JOIN das_scheme_groups sg ON sg.scheme_id = s.id "
                "LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.scheme_group_id "
                "WHERE s.id = %1 AND sgu.user_id = %2";

        Base& db = Base::get_thread_local_instance();
        QSqlQuery q = db.exec(sql.arg(scheme_id).arg(user_id));
        if (q.next())
        {
            std::set<uint32_t> extending_ids;
            if (!q.isNull(1))
                extending_ids.insert(q.value(1).toUInt());
            return Scheme_Info{q.value(0).toUInt(), std::move(extending_ids)}; // TODO: get array from specific table for extending ids
        }
    }

    return {};
}

void Scheme::get_dig_status(served::response &res, const served::request &req)
{
    const Scheme_Info scheme = get_info(req);

    Scheme_Status scheme_status;

    std::future<void> scheme_status_task = std::async(std::launch::async, [this, &scheme, &scheme_status]() -> void
    {
        QMetaObject::invokeMethod(dbus_iface_, "get_scheme_status", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(Scheme_Status, scheme_status),
            Q_ARG(uint32_t, scheme.id()));
    });

    Base& db = Base::get_thread_local_instance();
    QString group_title, sql;
    QStringList status_column_names = DIG_Status::table_column_names();
    QJsonArray j_array;

    scheme_status_task.get();

    if ((scheme_status.connection_state_ & ~CS_FLAGS) >= CS_CONNECTED_JUST_NOW)
    {
        if (!scheme_status.status_set_.empty())
        {
            sql = "SELECT g.id, s.name, g.title, gt.title "
                  "FROM das_device_item_group g "
                  "LEFT JOIN das_section s ON s.id = g.section_id "
                  "LEFT JOIN das_dig_type gt ON gt.id = g.type_id "
                  "WHERE g." + scheme.ids_to_sql() + " AND g.id IN (";

            for (const DIG_Status& status: scheme_status.status_set_)
                sql += QString::number(status.group_id()) + ',';
            sql.replace(sql.size() - 1, 1, QChar(')'));

            std::map<uint32_t, QString> group_title_map;
            QSqlQuery q = db.exec(sql);
            while(q.next())
            {
                group_title = q.value(2).toString();
                if (group_title.isEmpty())
                {
                    group_title = q.value(3).toString();
                }
                group_title.insert(0, q.value(1).toString() + ' ');
                group_title_map.emplace(q.value(0).toUInt(), group_title);
            }

            QJsonObject j_obj;
            for (const DIG_Status& status: scheme_status.status_set_)
            {
                fill_json_object(j_obj, status, status_column_names);
                j_obj["args"] = QJsonArray::fromStringList(status.args());
                j_obj.insert("title", group_title_map[status.group_id()]);
                j_array.push_back(j_obj);
            }
        }
    }
    else
    {
        sql = "SELECT gs.id, gs.timestamp_msecs, gs.user_id, gs.group_id, gs.status_id, gs.args, "
              "s.name, g.title, gt.title "
              "FROM das_dig_status gs "
              "LEFT JOIN das_device_item_group g ON g.id = gs.group_id "
              "LEFT JOIN das_section s ON s.id = g.section_id "
              "LEFT JOIN das_dig_type gt ON gt.id = g.type_id "
              "WHERE gs.scheme_id = " + QString::number(scheme.id());

        QSqlQuery q = db.exec(sql);
        while(q.next())
        {
            group_title = q.value(5).toString();
            if (group_title.isEmpty())
                group_title = q.value(6).toString();
            group_title.insert(0, q.value(4).toString() + ' ');

            QJsonObject j_obj = get_json_object<DIG_Status>(q, status_column_names);
            j_obj.insert("title", group_title);
            j_array.push_back(j_obj);
        }
    }

    QJsonObject j_obj;
    j_obj.insert("items", j_array);
    j_obj.insert("connection", static_cast<int>(scheme_status.connection_state_));

    res.set_header("Content-Type", "application/json");
    res << QJsonDocument(j_obj).toJson().toStdString();
}

void Scheme::get_dig_status_type(served::response &res, const served::request &req)
{
    const Scheme_Info scheme = get_info(req);

    res.set_header("Content-Type", "application/json");
    res << gen_json_list<DIG_Status_Type>("WHERE " + scheme.ids_to_sql());
}

void Scheme::get_device_item_value(served::response &res, const served::request &req)
{
    const Scheme_Info scheme = get_info(req);

    QVector<Device_Item_Value> unsaved_values;

    std::future<void> unsaved_values_task = std::async(std::launch::async, [this, &scheme, &unsaved_values]() -> void
    {
        QMetaObject::invokeMethod(dbus_iface_, "get_device_item_values", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVector<Device_Item_Value>, unsaved_values),
            Q_ARG(uint32_t, scheme.id()));
    });

    Base& db = Base::get_thread_local_instance();
    QVector<Device_Item_Value> values = db_build_list<Device_Item_Value>(
                db, "WHERE scheme_id=" + QString::number(scheme.id()));

    auto fill_obj = [](QJsonObject& obj, const Device_Item_Value& value)
    {
        obj.insert("ts", value.timestamp_msecs());
        obj.insert("user_id", static_cast<int>(value.user_id()));
        obj.insert("raw", QJsonValue::fromVariant(value.raw_value()));
        if (value.is_big_value())
        {
            obj.insert("display", value.value().toString().left(16));
            obj.insert("raw_value", value.raw_value().toString().left(16));
        }
        else
        {
            obj.insert("display", QJsonValue::fromVariant(value.value()));
            obj.insert("raw_value", QJsonValue::fromVariant(value.raw_value()));
        }
        obj.insert("value", QJsonValue::fromVariant(value.value()));
    };

    QJsonArray j_array;
    unsaved_values_task.get();
    QVector<Device_Item_Value>::iterator unsaved_it;

    for (const Device_Item_Value& value: values)
    {
        QJsonObject j_obj;
        j_obj.insert("id", static_cast<int>(value.item_id()));

        unsaved_it = unsaved_values.begin();
        while (true)
        {
            if (unsaved_it == unsaved_values.end())
            {
                fill_obj(j_obj, value);
                break;
            }
            else if (unsaved_it->item_id() == value.item_id())
            {
                fill_obj(j_obj, *unsaved_it);
                unsaved_values.erase(unsaved_it);
                break;
            }
            else
            {
                ++unsaved_it;
            }
        }

        j_array.push_back(j_obj);
    }

    res.set_header("Content-Type", "application/json");
    res << QJsonDocument(j_array).toJson().toStdString();
}

void Scheme::get_disabled_status(served::response &res, const served::request &req)
{
    Auth_Middleware::check_permission("view_disabled_status");

    const Scheme_Info scheme = get_info(req);

    uint32_t dig_id = stoa_or(req.query["dig_id"]);

    const QString sql = "WHERE %1 AND (dig_id IS NULL OR dig_id = %2)";

    res.set_header("Content-Type", "application/json");
    res << gen_json_list<DB::Disabled_Status>(sql.arg(scheme.ids_to_sql()).arg(dig_id));
}

void Scheme::del_disabled_status(served::response &res, const served::request &req)
{
    picojson::value val;
    const std::string err = picojson::parse(val, req.body());
    if (!err.empty() || !val.is<picojson::array>())
        throw served::request_error(served::status_4XX::BAD_REQUEST, err);

    std::set<uint32_t> id_set;
    uint32_t id;
    auto checkAdd = [&id_set, &id](const picojson::value& value)
    {
        if (value.is<double>())
        {
            id = static_cast<uint32_t>(value.get<double>());
            if (id)
            {
                id_set.insert(id);
                return true;
            }
        }
        return false;
    };

    const picojson::array& arr = val.get<picojson::array>();
    for (const picojson::value& value: arr)
    {
        if (!checkAdd(value) && value.is<picojson::object>())
        {
            const picojson::object& obj = value.get<picojson::object>();
            auto it = obj.find("id");
            if (it != obj.cend())
                checkAdd(it->second);
        }
    }

    if (id_set.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Failed find item for delete");

    Auth_Middleware::check_permission("delete_disabled_status");
    const Scheme_Info scheme = get_info(req);

    QString where = "scheme_id=";
    where += QString::number(scheme.id());
    where += " AND ";
    where += get_db_field_in_sql("id", id_set);

    Base& db = Base::get_thread_local_instance();
    db.del(db_table_name<DB::Disabled_Status>(), where);

    res.set_status(204);
}

inline QVariant pico_to_qvariant(const picojson::value& value)
{
    if (value.is<bool>())
        return value.get<bool>();
    else if (value.is<int64_t>())
        return QVariant::fromValue(value.get<int64_t>());
    else if (value.is<double>())
        return value.get<double>();
    else if (value.is<std::string>())
        return QString::fromStdString(value.get<std::string>());
    return {};
}

template<typename T>
T obj_from_pico(const picojson::object& item, const Helpz::DB::Table& table)
{
    T obj;

    int i;
    for (const auto& it: item)
    {
        i = table.field_names().indexOf(QString::fromStdString(it.first));
        if (i < 0 || i >= table.field_names().size())
            throw std::runtime_error("Unknown property: " + it.first);
        T::value_setter(obj, i, pico_to_qvariant(it.second));
    }

    return obj;
}

template<typename T>
T obj_from_pico(const picojson::object& item)
{
    const auto table = db_table<T>();
    return obj_from_pico<T>(item, table);
}

template<typename T>
T obj_from_pico(const picojson::value& value, const Helpz::DB::Table& table)
{
    if (!value.is<picojson::object>())
        throw std::runtime_error("Is not an object");
    return obj_from_pico<T>(value.get<picojson::object>(), table);
}

template<typename T>
T obj_from_pico(const picojson::value& value)
{
    const auto table = db_table<T>();
    return obj_from_pico<T>(value, table);
}

void Scheme::add_disabled_status(served::response &res, const served::request &req)
{
    picojson::value val;
    const std::string err = picojson::parse(val, req.body());
    if (!err.empty() || !val.is<picojson::array>())
        throw served::request_error(served::status_4XX::BAD_REQUEST, err);

    Auth_Middleware::check_permission("add_disabled_status");
    const Scheme_Info scheme = get_info(req);

    QVariantList values, tmp_values;

    Table table = db_table<DB::Disabled_Status>();

    const picojson::array& arr = val.get<picojson::array>();
    for (const picojson::value& value: arr)
    {
        DB::Disabled_Status item = obj_from_pico<DB::Disabled_Status>(value, table);
        item.set_scheme_id(scheme.id());

        tmp_values = DB::Disabled_Status::to_variantlist(item);
        tmp_values.removeFirst();
        values += tmp_values;
    }

    if (values.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Failed find item for insert");

    table.field_names().removeFirst();
    QString sql = "INSERT INTO " + table.name() + '(' + table.field_names().join(',') + ") VALUES" +
            Base::get_q_array(table.field_names().size(), values.size() / table.field_names().size());

    Base& db = Base::get_thread_local_instance();
    db.exec(sql, values);

    res.set_status(204);
}

void Scheme::get(served::response &res, const served::request &req)
{
    res.set_header("Content-Type", "application/json");
    res << "{ \"content\": \"Hello, " << req.params["scheme_id"] << "!\" }\n";

    std::cerr << "get scheme" << std::endl;
}

void Scheme::get_list(served::response &res, const served::request &req)
{
    Filter::Result filter = get_filter_result<DB::Scheme>(req.query, false);

    const uint32_t user_id = Auth_Middleware::get_thread_local_user().id_;

    QString suffix =
            "LEFT JOIN das_scheme_groups sg ON sg.scheme_id = s.id "
            "LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.scheme_group_id "
            "WHERE sgu.user_id = ";

    suffix += QString::number(user_id);
    suffix += filter.suffix_;
    suffix += " GROUP BY s.id";

    res.set_header("Content-Type", "application/json");
    res << gen_json_list<DB::Scheme>(suffix, filter.values_);
}

void Scheme::create(served::response &res, const served::request &req)
{
    picojson::value val;
    const std::string err = picojson::parse(val, req.body());
    if (!err.empty() || !val.is<picojson::object>())
        throw served::request_error(served::status_4XX::BAD_REQUEST, err);

    picojson::object& obj = val.get<picojson::object>();
    auto s_groups_it = obj.find("scheme_groups");
    if (s_groups_it == obj.cend() || !s_groups_it->second.is<picojson::array>())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "scheme_groups is required");

    const picojson::array& scheme_groups = s_groups_it->second.get<picojson::array>();
    if (scheme_groups.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "scheme_groups can't be empty");

    Auth_Middleware::check_permission("add_scheme");

    DB::Scheme scheme = from_json<DB::Scheme>(obj);
    scheme.set_last_usage(QDateTime::currentDateTimeUtc());
    scheme.set_version("");
    scheme.set_using_key(QUuid().toString(QUuid::Id128));

    const uint32_t user_id = Auth_Middleware::get_thread_local_user().id_;

    Base& db = Base::get_thread_local_instance();

    QString sql =
            "SELECT * FROM das_scheme_groups sg "
            "LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.scheme_group_id "
            "WHERE sgu.user_id = %1 AND sg.scheme_id = %2 "
            "LIMIT 1";

    if (scheme.parent_id())
    {
        auto q = db.exec(sql.arg(user_id).arg(scheme.parent_id()));
        if (!q.isActive() || !q.next())
            throw served::request_error(served::status_4XX::BAD_REQUEST, "Can't find scheme id: " + std::to_string(scheme.parent_id()));
    }
    else
    {
        // TODO: check can create scheme controller (parent scheme)
    }

    sql = "SELECT COUNT(*) FROM das_scheme_group_user sgu WHERE sgu.user_id = ";
    sql += QString::number(user_id);
    sql += " AND sgu.group_id IN (";

    for (const picojson::value& s_group: scheme_groups)
    {
        sql += QString::number(s_group.get<int64_t>());
        sql += ',';
    }
    sql.replace(sql.size()-1, 1, ')');

    QSqlQuery q = db.exec(sql);
    if (!q.isActive() || !q.next() || q.value(0).toUInt() == 0)
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Can't find some scheme group");

    QVariant new_id;
    if (!db.insert(db_table<DB::Scheme>(), DB::Scheme::to_variantlist(scheme), &new_id))
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, "Can't create new scheme: " + scheme.name().toStdString());

    const QString new_id_str = new_id.toString();
    sql = "INSERT INTO das_scheme_groups(scheme_id, scheme_group_id) VALUES";
    for (const picojson::value& s_group: scheme_groups)
    {
        sql += '(';
        sql += new_id_str;
        sql += ',';
        sql += QString::number(s_group.get<int64_t>());
        sql += "),";
    }
    sql.replace(sql.size()-1, 1, ';');
    q = db.exec(sql);
    if (!q.isActive())
    {
        db.exec("DELETE FROM das_scheme_groups WHERE scheme_id = " + new_id_str);
        db.exec("DELETE FROM das_scheme WHERE id = " + new_id_str);
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, "Can't create new scheme: " + scheme.name().toStdString() + " failed add to groups");
    }

    obj["id"] = picojson::value(new_id.value<int64_t>());
    res << val.serialize();
    std::cerr << "Scheme::create: " << val.serialize() << std::endl;
}

void Scheme::set_name(served::response &res, const served::request &req)
{
    const Scheme_Info scheme = get_info(req);

    picojson::value val;
    const std::string err = picojson::parse(val, req.body());
    if (!err.empty() || !val.is<picojson::object>())
        throw served::request_error(served::status_4XX::BAD_REQUEST, err);

    const picojson::object& obj = val.get<picojson::object>();
    const std::string scheme_name = obj.at("name").get<std::string>();

    std::cerr << "set_name: " << scheme_name << std::endl;

    const uint32_t user_id = Auth_Middleware::get_thread_local_user().id_;

    QMetaObject::invokeMethod(dbus_iface_, "set_scheme_name", Qt::QueuedConnection,
        Q_ARG(uint32_t, scheme.id()), Q_ARG(uint32_t, user_id), Q_ARG(QString, QString::fromStdString(scheme_name)));

    res << "{\"result\": true}\n";
}

void Scheme::copy(served::response &res, const served::request &req)
{
    picojson::value val;
    const std::string err = picojson::parse(val, req.body());
    if (!err.empty() || !val.is<picojson::object>())
        throw served::request_error(served::status_4XX::BAD_REQUEST, err);

    const picojson::object& obj = val.get<picojson::object>();
    const uint32_t dest_scheme_id = obj.at("scheme_id").get<int64_t>();
    const bool is_dry_run = obj.at("dry_run").get<bool>();

    const Scheme_Info scheme = Scheme::get_info(req);
    const Scheme_Info dest_scheme = Scheme::get_info(dest_scheme_id);
    if (!dest_scheme.id()
        || !scheme.extending_scheme_ids().empty()
        || !dest_scheme.extending_scheme_ids().empty())
    {
        const std::string error_msg = fmt::format("Not possible copy scheme {}{} to {}{}.",
                                                  scheme.id(), scheme.extending_scheme_ids(),
                                                  dest_scheme_id, dest_scheme.extending_scheme_ids());
        throw served::request_error(served::status_4XX::BAD_REQUEST, error_msg);
    }

    Auth_Middleware::check_permission("change_scheme");

    std::cout << "Copy " << scheme.id() << " to " << dest_scheme.id() << std::endl;

    Scheme_Copier copier(scheme.id(), dest_scheme.id(), is_dry_run);

    picojson::object data;

    auto get_counter_name = [](int index) -> std::string
    {
        using T = Scheme_Copier::Item::Counter_Type;
        switch (index) {
        case T::SCI_DELETED:        return "deleted";
        case T::SCI_DELETE_ERROR:   return "delete_error";
        case T::SCI_INSERTED:       return "inserted";
        case T::SCI_INSERT_ERROR:   return "insert_error";
        case T::SCI_UPDATED:        return "updated";
        case T::SCI_UPDATE_ERROR:   return "update_error";
        default:
            break;
        }
        return "unknown";
    };

    bool is_empty;
    int64_t counter;
    for (const std::pair<std::string, Scheme_Copier::Item>& item: copier.result_)
    {
        picojson::object data_item;

        is_empty = true;
        for (int i = 0; i < Scheme_Copier::Item::SCI_COUNT; ++i)
        {
            counter = item.second.counter_[i];

            if (!counter)
                continue;

            is_empty = false;

            data_item.emplace(get_counter_name(i), picojson::value(counter));
        }

        if (!is_empty)
            data.emplace(item.first, picojson::value(data_item));
    }

    picojson::object res_obj;
    res_obj.emplace("result", picojson::value(true));
    res_obj.emplace("data", picojson::value(std::move(data)));
    res << picojson::value(std::move(res_obj)).serialize();
}

} // namespace Rest
} // namespace Das
