#include <iostream>

#include <served/request_error.hpp>

#include <plus/das/jwt_helper.h>
#include <plus/das/database.h>

#include <Helpz/db_base.h>

#include "rest_helper.h"
#include "auth_middleware.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

/*static*/ Auth_Middleware* Auth_Middleware::_obj = nullptr;

thread_local Auth_User thread_local_user;
thread_local std::shared_ptr<JWT_Helper> jwt_helper;
static std::string jwt_secret_key;

/*static*/ const Auth_User& Auth_Middleware::get_thread_local_user() { return thread_local_user; }

/*static*/ void Auth_Middleware::check_permission(const std::string &permission)
{
    const uint32_t user_id = get_thread_local_user().id_;
    if (!DB::Helper::check_permission(user_id, permission))
        throw served::request_error(served::status_4XX::FORBIDDEN, "You don't have permission. " + permission);
}

/*static*/ bool Auth_Middleware::has_permission(const std::string &permission)
{
    try {
        check_permission(permission);
    } catch (...) {
        return false;
    }
    return true;
}

/*static*/ void Auth_Middleware::set_jwt_secret_key(const std::string &key) { jwt_secret_key = key; }

Auth_Middleware::Auth_Middleware(std::chrono::seconds token_timeout,
                                 const std::vector<std::string>& exclude_path) :
    exclude_path_vect_(exclude_path),
    _token_timeout(token_timeout)
{
    _obj = this;
}

Auth_Middleware::Auth_Middleware(const Auth_Middleware &o) :
    exclude_path_vect_(o.exclude_path_vect_),
    _token_timeout(o._token_timeout)
{
    _obj = this;
}

Auth_Middleware::Auth_Middleware(Auth_Middleware &&o) :
    exclude_path_vect_(std::move(o.exclude_path_vect_)),
    _token_timeout(std::move(o._token_timeout))
{
    _obj = this;
}

void Auth_Middleware::operator ()(served::response&, const served::request& req)
{
    thread_local_user.clear();

//    std::cout << served::method_to_string(req.method()) << " Auth_Middleware: " << req.url().URI() << std::endl;
    const std::string req_path = req.url().path();
    if (is_exclude(req_path))
        return;

    check_token(req);
    // Authorization: JWT eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6ImRldiIsImV4cCI6MTU2OTYyNzMzNSwidXNlcl9pZCI6MjcsImVtYWlsIjoiIiwidGVhbXMiOls5XSwib3JpZ19pYXQiOjE1Njk0Njk4MDZ9.4YqIbAnxIALYdB0Knw1cajsxmgv6BnzYgqDO0ck8oxg
}

/*static*/ std::string Auth_Middleware::create_token(uint32_t user_id, const std::string& username, const std::string& session_id)
{
    return get_jwt_helper()->create(user_id, username, _obj->_token_timeout, session_id);
}

/*static*/ JWT_Helper *Auth_Middleware::get_jwt_helper()
{
    if (!jwt_helper)
        jwt_helper = std::make_shared<JWT_Helper>(jwt_secret_key);
    return jwt_helper.get();
}

bool Auth_Middleware::is_exclude(const std::string &url_path)
{
    for (const std::string& path: exclude_path_vect_)
        if (path == url_path)
            return true;
    return false;
}

void Auth_Middleware::check_token(const served::request &req)
{
    std::string token = req.header("Authorization");
    if (token.size() <= 4)
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Token is too small");

    token.replace(0, 4, std::string());

    try
    {
        const std::string json_raw = get_jwt_helper()->decode_and_verify(token);
        const picojson::object obj = Helper::parse_object(json_raw);
        thread_local_user.id_ = obj.at("user_id").get<int64_t>();

        auto sid_it = obj.find("session_id");
        if (sid_it != obj.cend())
            thread_local_user._session_id = std::move(sid_it->second.get<std::string>());
    }
    catch(const std::exception& e)
    {
        std::cerr << "JWT Exception: " << e.what() << " token: " << token << std::endl;
        throw served::request_error(served::status_4XX::UNAUTHORIZED, e.what());
    }
}

} // namespace Rest
} // namespace Das
