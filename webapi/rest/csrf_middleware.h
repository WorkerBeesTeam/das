#ifndef DAS_REST_CSRF_MIDDLEWARE_H
#define DAS_REST_CSRF_MIDDLEWARE_H

#include <served/request.hpp>
#include <served/response.hpp>

namespace Das {
namespace Rest {

class CSRF_Middleware
{
public:
    CSRF_Middleware();
    void operator ()(served::response &res, const served::request & req);
private:
    void check_cookie_token(const std::string& token, served::response &res);
    std::string get_cookie_token(const served::request & req) const;
    std::string unsalt_csrf_token(const std::string& token);
    bool is_valid_csrf_token(const std::string& token);
    bool check_csrf_token(const std::string& token, const std::string& token_request);
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CSRF_MIDDLEWARE_H
