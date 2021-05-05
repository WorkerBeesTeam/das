#ifndef DAS_REST_AUTH_MIDDLEWARE_H
#define DAS_REST_AUTH_MIDDLEWARE_H

#include <vector>
#include <set>
#include <chrono>

#include <served/request.hpp>
#include <served/response.hpp>

namespace Das {

class JWT_Helper;

namespace Rest {

struct Auth_User
{
    uint32_t id_;
    std::string _session_id;

    void clear()
    {
        id_ = 0;
        _session_id.clear();
    }
};

class Auth_Middleware
{
    static Auth_Middleware* _obj;
public:
    static const Auth_User& get_thread_local_user();

    static void check_permission(const std::string& permission);
    static bool has_permission(const std::string& permission);

    static void set_jwt_secret_key(const std::string& key);

    Auth_Middleware(std::chrono::seconds token_timeout,
                    const std::vector<std::string>& exclude_path = {});
    Auth_Middleware(const Auth_Middleware& o);
    Auth_Middleware(Auth_Middleware&& o);
    Auth_Middleware& operator=(Auth_Middleware&&) = delete;
    Auth_Middleware& operator=(const Auth_Middleware&) = delete;
    void operator ()(served::response &, const served::request & req);

    static std::string create_token(uint32_t user_id, const std::string &username, const std::string &session_id = {});
private:
    static JWT_Helper* get_jwt_helper();

    bool is_exclude(const std::string& url_path);
    void check_token(const served::request& req);

    std::vector<std::string> exclude_path_vect_;
    std::chrono::seconds _token_timeout;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_AUTH_MIDDLEWARE_H
