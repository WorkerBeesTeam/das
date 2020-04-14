#include <iostream>

#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

#include <served/served.hpp>
#include <served/status.hpp>
#include <served/request_error.hpp>

#include "multipart_form_data_parser.h"
#include "csrf_middleware.h"
#include "auth_middleware.h"
#include "rest_scheme_group.h"
#include "rest_scheme.h"
#include "rest.h"

namespace Das {
namespace Rest {

Q_LOGGING_CATEGORY(Rest_Log, "rest")


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

        auto scheme_groups = std::make_shared<Scheme_Group>(mux);
        auto scheme = std::make_shared<Scheme>(mux, dbus_iface);

        mux.handle("write_item_file").put([](served::response &res, const served::request &req)
        {
            Multipart_Form_Data_Parser parser(req);
//            if (parser.parse())
            {

            }
            qCDebug(Rest_Log) << QByteArray::fromStdString(req.body());
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
    catch(const std::exception& e)
    {
        qCCritical(Rest_Log) << "Failure:" << e.what();
    }
    catch(...)
    {
        qCCritical(Rest_Log) << "Served unknown exception:" << strerror(errno);
    }

    server_ = nullptr;
//    const QByteArray token("MZ57f6dHSAruWf4r6errrLLLodZiZ2dpPSflFHUq10aKDfEliNBZyE8JPqyuYiAL");
//    const QByteArray token_request("MZ57f6dHSAruWf4r6errrLLLodZiZ2dpPSflFHUq10aKDfEliNBZyE8JPqyuYiAL");
    //    std::cerr << "Token is " << std::boolalpha << check_csrf_token(token, token_request) << std::endl;
}

} // namespace Rest
} // namespace Das
