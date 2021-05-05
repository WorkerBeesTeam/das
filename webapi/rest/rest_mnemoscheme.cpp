#include <Helpz/zfile.h>

#include "auth_middleware.h"
#include "rest_helper.h"
#include "rest_scheme.h"
#include "rest_mnemoscheme.h"

namespace Das {
namespace Rest {

Mnemoscheme::Mnemoscheme(served::multiplexer &mux, const std::string &scheme_path)
{
    const std::string url = scheme_path + "/mnemoscheme/";
    mux.handle(url + "{elem_id:[0-9]+}/")
            .get([this](served::response &res, const served::request &req) { get_item(res, req); });
    mux.handle(url)
            .get([this](served::response &res, const served::request &req) { get_list(res, req); });
}

void Mnemoscheme::get_list(served::response &res, const served::request &req)
{
    Auth_Middleware::check_permission("view_mnemoscheme");

    picojson::array json;

    const Scheme_Info scheme = Scheme::get_info(req);
    if ((scheme.extending_scheme_ids().empty() && scheme.id() == 1)
      || !scheme.extending_scheme_ids().empty() && *scheme.extending_scheme_ids().begin() == 1)
    {
        picojson::object obj;
        obj.emplace("id", static_cast<int64_t>(1));
        obj.emplace("title", "Пользовательская");
        json.emplace_back(std::move(obj));

        picojson::object obj2;
        obj2.emplace("id", static_cast<int64_t>(1));
        obj2.emplace("title", "Техническая");
        json.emplace_back(picojson::value{std::move(obj2)});
    }

    res.set_header("Content-Type", "application/json");
    res << picojson::value{std::move(json)}.serialize();
}

void Mnemoscheme::get_item(served::response &res, const served::request &req)
{
    uint32_t id = Helper::get_element_id(req);
    Auth_Middleware::check_permission("view_mnemoscheme");

    const Scheme_Info scheme = Scheme::get_info(req);
    if ((scheme.extending_scheme_ids().empty() && scheme.id() == 1)
     || (!scheme.extending_scheme_ids().empty() && *scheme.extending_scheme_ids().begin() == 1))
    {
        const std::string data = Helpz::File::read_all("/opt/mnemoscheme.svg");
        res.set_header("Content-Type", "image/svg; charset=UTF-8");
        res.set_body(data);
    }
    else
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Unknown element id");
}

} // namespace Rest
} // namespace Das
