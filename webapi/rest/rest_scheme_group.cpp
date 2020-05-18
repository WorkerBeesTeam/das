
#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

#include <Helpz/db_base.h>

#include "auth_middleware.h"
#include "rest_scheme_group.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

Scheme_Group::Scheme_Group(served::multiplexer &mux)
{
    const std::string scheme_group_path = "/scheme_group/";
    mux.handle(scheme_group_path)
            .get([this](served::response& res, const served::request& req) { get_list(res, req); });
}

void Scheme_Group::get_list(served::response &res, const served::request &/*req*/)
{
    const uint32_t user_id = Auth_Middleware::get_thread_local_user().id_;

    const QString sql =
            "SELECT sg.* FROM das_scheme_group sg "
            "LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.id "
            "WHERE sgu.user_id = %1 "
            "GROUP BY sg.id";

    picojson::array s_groups;

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql.arg(user_id));
    while (q.next())
    {
        picojson::object s_group;
        s_group["id"] = picojson::value(q.value(0).value<int64_t>());
        s_group["name"] = picojson::value(q.value(1).toString().toStdString());

        s_groups.push_back(picojson::value{std::move(s_group)});
    }

    const picojson::value j_value{std::move(s_groups)};
    res.set_header("Content-Type", "application/json");
    res << j_value.serialize();
}

} // namespace Rest
} // namespace Das
