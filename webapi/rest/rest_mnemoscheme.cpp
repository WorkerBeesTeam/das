
#include <QCryptographicHash>

#include <Helpz/zfile.h>

#include <Das/db/mnemoscheme.h>

#include "auth_middleware.h"
#include "json_helper.h"
#include "rest_helper.h"
#include "rest_config.h"
#include "rest_scheme.h"
#include "rest_mnemoscheme.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

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

    const Scheme_Info scheme = Scheme::get_info(req);

    Table table = db_table<DB::Mnemoscheme>();
    table.field_names().removeAt(DB::Mnemoscheme::COL_scheme_id);
    table.field_names().removeAt(DB::Mnemoscheme::COL_hash);

    res << gen_json_list2(table, "WHERE " + scheme.ids_to_sql());
    res.set_header("Content-Type", "application/json");
}

void Mnemoscheme::get_item(served::response &res, const served::request &req)
{
    uint32_t id = Helper::get_element_id(req);
    Auth_Middleware::check_permission("view_mnemoscheme");

    const Scheme_Info scheme = Scheme::get_info(req);

    Base& db = Base::get_thread_local_instance();

    QSqlQuery q = db.select(db_table<DB::Mnemoscheme>(), "WHERE " + scheme.ids_to_sql() + " AND id=?", {id});
    if (!q.isActive() || !q.next())
//        throw served::request_error(served::status_4XX::BAD_REQUEST, "Unknown element id"); // TODO: if debug mode false.
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());

    const DB::Mnemoscheme item = db_build<DB::Mnemoscheme>(q);

    const std::string file_path = Rest::Config::instance()._blob_dir_path + "/mnemoscheme/" + std::to_string(item.id()) + ".svg";
    const std::string data = Helpz::File::read_all(file_path);
    if (data.empty())
        throw served::request_error(served::status_4XX::NOT_FOUND, "Not found.");

    QCryptographicHash hash_func(QCryptographicHash::Sha1);
    hash_func.addData(data.data(), data.size());
    if (QByteArray::fromHex(item.hash()) != hash_func.result())
        throw served::request_error(served::status_4XX::NOT_FOUND, "Not found.");

    res.set_header("Content-Type", "image/svg; charset=UTF-8");
    res.set_body(data);
}

} // namespace Rest
} // namespace Das
