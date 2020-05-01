
#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QSqlError>
#include <QUuid>

#include <Helpz/db_base.h>

//#include <Das/section.h>
#include <Das/db/scheme.h>
#include <Das/db/dig_status_type.h>
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

    mux.handle(scheme_path + "/dig_status").get([this](served::response& res, const served::request& req) { get_dig_status(res, req); });
    mux.handle(scheme_path + "/dig_status_type").get([this](served::response& res, const served::request& req) { get_dig_status_type(res, req); });
    mux.handle(scheme_path + "/device_item_value").get([this](served::response& res, const served::request& req) { get_device_item_value(res, req); });
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
            return {q.value(0).toUInt(), q.value(1).toUInt()};
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
                  "WHERE g.scheme_id = " + QString::number(scheme.parent_id_or_id()) + " AND g.id IN (";

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
    res << gen_json_list<DIG_Status_Type>("WHERE scheme_id=" + QString::number(scheme.parent_id_or_id()));
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
    if (scheme.parent_id() || !dest_scheme.id() || dest_scheme.parent_id())
    {
        const std::string error_msg = "Not possible copy scheme " + std::to_string(scheme.id()) + " to " + std::to_string(dest_scheme_id);
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
