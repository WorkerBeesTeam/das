#include <iostream>
#include <string>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <served/served.hpp>
#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#define PICOJSON_USE_INT64
#include <plus/jwt-cpp/include/jwt-cpp/picojson.h>

#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include <Das/section.h>
#include <Das/db/dig_status_type.h>
#include <Das/db/dig_status.h>
#include <Das/db/device_item_value.h>
#include <plus/das/scheme_info.h>
#include <dbus/dbus_interface.h>

#include "csrf_middleware.h"
#include "auth_middleware.h"
#include "restful.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

class Multipart_Form_Data_Parser
{
public:
    Multipart_Form_Data_Parser(const served::request& req) :
        req_(req)
    {
//         "multipart/form-data; "
//         "boundary=------------------------000000000000000; "
//         "boundary=------------------------14609ac75d1a8714";
        const std::string content_type = req.header("Content-Type");

        std::vector<std::string> content_type_list;
        boost::split(content_type_list, content_type, boost::is_any_of("; "));
        boost::to_lower(content_type_list.front());
        if (content_type_list.front() == "multipart/form-data")
        {
//            "--------------------------14609ac75d1a8714\r\n"
//            "Content-Disposition: attachment; name=\"myfilename\"; filename=\"test.bin\"\r\n"
//            "Content-Type: application/octet-stream\r\n\r\n"
//            "DATA\r\n"
//            "--------------------------14609ac75d1a8714--\r\n";
            const std::string body = req.body();

            std::vector<std::string> pv_list;
            for (auto it = ++content_type_list.cbegin(); it != content_type_list.cend(); ++it)
            {
                pv_list.clear();
                boost::split(pv_list, *it, [](char c){ return c == '='; });
                boost::to_lower(pv_list.front());
                if (pv_list.front() == "boundary")
                {
                    parse_part(body, pv_list.back());
                }
            }
        }

        qDebug() << "write_item_file" << req.body().size() << QString::fromStdString(content_type);
    }
private:
    void parse_part(const std::string& body, const std::string& boundary)
    {
        std::size_t pos = body.find(boundary);
        if (pos != std::string::npos)
        {
            pos += boundary.size() + 2; // \r\n
            std::size_t end_pos = body.find(boundary, pos);
            if (end_pos != std::string::npos)
            {
                end_pos -= 4; // \r\n--
                const std::string rn = "\r\n";

                pos = body.find(rn, pos);
                if (pos != std::string::npos)
                {
                    /* TODO:
                     */
                    body.substr(0, pos);
                }
            }
        }
    }
    const served::request& req_;
};

Restful::Restful(DBus::Interface* dbus_iface, std::shared_ptr<JWT_Helper> jwt_helper, const Config &config) :
    server_(nullptr),
    thread_(&Restful::run, this, dbus_iface, std::move(jwt_helper), config)
{
}

Restful::~Restful()
{
    stop();
    join();
}

void Restful::stop()
{
    if (server_)
        server_->stop();
}

void Restful::join()
{
    if (thread_.joinable())
        thread_.join();
}

template<typename T>
void fill_json_object(QJsonObject& obj, const T& item, const QStringList& names)
{
    for (int i = 0; i < T::COL_COUNT; ++i)
    {
        obj.insert(names.at(i), QJsonValue::fromVariant(T::value_getter(item, i)));
    }
}

template<typename T>
QJsonObject get_json_object(const QSqlQuery& query, const QStringList& names)
{
    QJsonObject obj;

    if (query.record().count() >= T::COL_COUNT)
    {
        const T item = db_build<T>(query);
        fill_json_object(obj, item, names);
    }
    return obj;
}

template<typename T>
QJsonObject get_json_object(const QSqlQuery& query)
{
    const QStringList names = T::table_column_names();
    return get_json_object<T>(query, names);
}

template<typename T>
QJsonArray gen_json_array(const QString& suffix = QString())
{
    Base& db = Base::get_thread_local_instance();
    QStringList names = T::table_column_names();
    QJsonArray json_array;
    auto q = db.select(db_table<T>(), suffix);
    while(q.next())
    {
        json_array.push_back(get_json_object<T>(q, names));
    }

    return json_array;
}

template<typename T>
std::string gen_json_list(const QString& suffix = QString())
{
    return QJsonDocument(gen_json_array<T>(suffix)).toJson().toStdString();
}

// throw served::request_error(served::status_4XX::NOT_FOUND, "Scheme not found");

const char *get_scheme_base() { return "scheme"; }
std::string get_scheme_path_base() { return '/' + std::string(get_scheme_base()) + '/'; }

std::string get_scheme_path()
{
    return get_scheme_path_base() + '{' + get_scheme_base() + "_id:[0-9]+}";
}

Scheme_Info get_scheme(const served::request& req)
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
        }
    }

    throw served::request_error(served::status_4XX::NOT_FOUND, "Scheme not found");
    return {};
}

void Restful::run(DBus::Interface* dbus_iface, std::shared_ptr<JWT_Helper> jwt_helper, const Config &config)
{
    try
    {
        served::multiplexer mux;

        // register one or more handlers
        const std::string token_auth {"/token/auth"};
        mux.handle(token_auth).post([](served::response &res, const served::request &req)
        {
            picojson::value val;
            const std::string err = picojson::parse(val, req.body());
            if (!err.empty() || !val.is<picojson::object>())
                throw served::request_error(served::status_4XX::BAD_REQUEST, err);

            const picojson::object obj = val.get<picojson::object>();
            const std::string username = obj.at("username").get<std::string>();
            const std::string password = obj.at("password").get<std::string>();

            std::cout << username << " + " << password << std::endl;

            res.set_header("Content-Type", "application/json");
        });

        const std::string scheme_path = get_scheme_path();
        mux.handle(scheme_path + "/dig_status").get([dbus_iface](served::response& res, const served::request& req)
        {
            const Scheme_Info scheme = get_scheme(req);

            Scheme_Status scheme_status;

            std::future<void> scheme_status_task = std::async(std::launch::async, [dbus_iface, &scheme, &scheme_status]() -> void
            {
                QMetaObject::invokeMethod(dbus_iface, "get_scheme_status", Qt::BlockingQueuedConnection,
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
        });

        mux.handle(scheme_path + "/device_item_value").get([dbus_iface](served::response& res, const served::request& req)
        {
            const Scheme_Info scheme = get_scheme(req);

            QVector<Device_Item_Value> unsaved_values;

            std::future<void> unsaved_values_task = std::async(std::launch::async, [dbus_iface, &scheme, &unsaved_values]() -> void
            {
                QMetaObject::invokeMethod(dbus_iface, "get_device_item_values", Qt::BlockingQueuedConnection,
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
                obj.insert("display", QJsonValue::fromVariant(value.value()));
                obj.insert("raw_value", QJsonValue::fromVariant(value.raw_value()));
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
        });

        mux.handle(scheme_path + "/dig_status_type").get([](served::response& res, const served::request& req)
        {
            const Scheme_Info scheme = get_scheme(req);

            res.set_header("Content-Type", "application/json");
            res << gen_json_list<DIG_Status_Type>("WHERE scheme_id=" + QString::number(scheme.parent_id_or_id()));
        });

        mux.handle(scheme_path).get([](served::response& res, const served::request& req)
        {
            res.set_header("Content-Type", "application/json");
            res << "{ \"content\": \"Hello, " << req.params["scheme_id"] << "!\" }\n";

            std::cerr << "get scheme" << std::endl;
        });

        mux.handle("write_item_file").put([](served::response &res, const served::request &req)
        {
            Multipart_Form_Data_Parser parser(req);
//            if (parser.parse())
            {

            }
            qDebug() << QByteArray::fromStdString(req.body());
            res.set_status(204);
        });

        // register middleware / plugin
        mux.use_before(CSRF_Middleware());
        mux.use_before(Auth_Middleware(std::move(jwt_helper), {token_auth}));

        served::net::server server{config.address_, config.port_, mux};
        server_ = &server;

        std::cout << "Restful server start on " << config.address_ << ':' << config.port_ << std::endl;
        std::cout << "Try this example with:" << std::endl;
        std::cout << " curl http://localhost:" << config.port_ << "/served" << std::endl;

        server.run(config.thread_count_);
    }
    catch(...)
    {
        qCritical() << "Served EXCEPTION";
    }

    server_ = nullptr;
//    const QByteArray token("MZ57f6dHSAruWf4r6errrLLLodZiZ2dpPSflFHUq10aKDfEliNBZyE8JPqyuYiAL");
//    const QByteArray token_request("MZ57f6dHSAruWf4r6errrLLLodZiZ2dpPSflFHUq10aKDfEliNBZyE8JPqyuYiAL");
    //    std::cerr << "Token is " << std::boolalpha << check_csrf_token(token, token_request) << std::endl;
}

} // namespace Rest
} // namespace Das
