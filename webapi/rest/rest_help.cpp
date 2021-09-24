#include <filesystem>

#include <QRegularExpression>

#include <Helpz/db_base.h>
#include <Helpz/zfile.h>

#include <Das/db/help.h>

#include "auth_middleware.h"

#include "json_helper.h"
#include "rest_helper.h"
#include "rest_config.h"
#include "rest_scheme.h"
#include "rest_help.h"

namespace Das {
namespace Rest {

using namespace Helpz::DB;

Help::Help(served::multiplexer& mux, const std::string& scheme_path)
{
    register_handlers(mux, std::string{}, true);
    register_handlers(mux, scheme_path, false);
}

void Help::register_handlers(served::multiplexer& mux, const std::string& base_path, bool is_common)
{
    const std::string url = base_path + "/help/";
    const std::string item_url = url + "{elem_id:[0-9]+}/";

    mux.handle(item_url + "{blob_id:[0-9]+}/")
        .head([this, is_common](served::response &res, const served::request &req) { get_blob_head(res, req, is_common); })
        .get([this, is_common](served::response &res, const served::request &req) { get_blob(res, req, is_common); })
        .post([this, is_common](served::response &res, const served::request &req) { upsert_blob(res, req, is_common); });
    mux.handle(item_url)
        .get([this, is_common](served::response &res, const served::request &req) { get_item(res, req, is_common); })
        .del([this, is_common](served::response &res, const served::request &req) { del_item(res, req, is_common); });
    mux.handle(url)
        .get([this, is_common](served::response &res, const served::request &req) { get_tree(res, req, is_common); })
        .post([this, is_common](served::response &res, const served::request &req) { upsert_tree(res, req, is_common); });
}

void Help::get_tree(served::response &res, const served::request &req, bool is_common)
{
    Auth_Middleware::check_permission("view_help");
    QString sql = base_sql_where(req, is_common) + "ORDER BY parent_id";

    using T = DB::Help;
    res << gen_json_list<DB::Help>(sql, {}, {T::COL_id, T::COL_name, T::COL_parent_id});
    res.set_header("Content-Type", "application/json");
}

void Help::upsert_tree(served::response &res, const served::request &req, bool is_common)
{
    using T = DB::Help;
    const picojson::object elem_obj = Helper::parse_object(req.body());
    T elem = obj_from_pico<T>(elem_obj);

    if (is_common)
        elem.set_scheme_id(0);
    else
    {
        const Scheme_Info scheme = Scheme::get_info(req);
        elem.set_scheme_id(scheme.extending_scheme_ids().empty() ?
                               scheme.id() : *scheme.extending_scheme_ids().begin());
    }

    if (elem.id())
    {
        Auth_Middleware::check_permission("change_help");
        check_help_item_perms(req, is_common, elem.id());
    }
    else
        Auth_Middleware::check_permission("add_help");

    if (elem.parent_id())
        check_help_item_perms(req, is_common, elem.parent_id());

    Helpz::DB::Base& db = Helpz::DB::Base::get_thread_local_instance();
    if (elem.id())
    {
        QString sql = base_sql_where(req, is_common) + "AND id = " + QString::number(elem.id());
        if (!db.update(db_table<DB::Help>(), DB::Help::to_variantlist(elem), sql).isActive())
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());

        // Parse and remove
        std::set<QString> blobs;
        QRegularExpression re("<file (\d+)", QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
        auto i = re.globalMatch(elem.text());
        while (i.hasNext())
            blobs.insert(i.next().captured(1));

        const std::string base_path = help_item_path(elem.id());
        for(auto const& dir_entry: std::filesystem::directory_iterator{base_path})
            if (blobs.find(QString::fromStdString(std::filesystem::path{dir_entry}.filename())) == blobs.cend())
                std::filesystem::remove_all(dir_entry);
    }
    else
    {
        QVariant id;
        if (!db.insert(db_table<DB::Help>(), DB::Help::to_variantlist(elem), &id))
            throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
        elem.set_id(id.toUInt());
    }


    res.set_header("Content-Type", "application/json");
    {
        // Check
    }

    // Создаём или редактируем элемент. В теле должен быть полный { id?, name, text, parent_id }
    // Если редактируем то спарсить блобы и удалить те которые есть в папке, но не в text
}

void Help::get_item(served::response &res, const served::request &req, bool is_common)
{
    uint32_t id = Helper::get_element_id(req);
    Auth_Middleware::check_permission("view_help");
    QString sql = base_sql_where(req, is_common) + "AND id = " + QString::number(id);

    res << gen_json_list<DB::Help>(sql, {}, {DB::Help::COL_text});
    res.set_header("Content-Type", "application/json");
}

void Help::del_item(served::response &res, const served::request &req, bool is_common)
{
    uint32_t id = Helper::get_element_id(req);
    Auth_Middleware::check_permission("delete_help");

    // Удаляем из базы и всю папку с блобами

    Helpz::DB::Base& db = Helpz::DB::Base::get_thread_local_instance();
    QString sql = base_sql_where(req, is_common) + "AND id = " + QString::number(id);
    auto q = db.del(db_table_name<DB::Help>(), sql);
    if (!q.isActive())
//        throw served::request_error(served::status_4XX::BAD_REQUEST, "Unknown element id"); // TODO: if debug mode false.
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());

    if (q.numRowsAffected())
    {
        std::filesystem::remove_all(help_item_path(id));
        res.set_status(201);
    }
    else
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Unknown element id"); // TODO: if debug mode false.
}

void Help::get_blob_head(served::response &res, const served::request &req, bool is_common)
{
    get_blob(res, req, is_common, /*is_head=*/true);
}

void Help::get_blob(served::response &res, const served::request &req, bool is_common, bool is_head)
{
    uint32_t id = Helper::get_element_id(req);
    uint32_t blob_id = Helper::get_element_id(req);

    Auth_Middleware::check_permission("view_help");

    check_help_item_perms(req, is_common, id);

    const std::string file_path = help_item_path(id) + std::to_string(blob_id);
    if (!Helpz::File::exist(file_path))
        throw served::request_error(served::status_4XX::NOT_FOUND, "File not found");

    Helpz::File file{file_path};
    if (!file.open(Helpz::File::READ_ONLY))
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, file.last_error());

    uint32_t content_type_size = 0;
    file.read(reinterpret_cast<char*>(&content_type_size), 8);
    const std::string content_type = file.read(content_type_size);
    res.set_header("Content-Type", content_type);

    if (!is_head)
    {
        const std::string data = file.read(file.size() - content_type_size - 8);
        res.set_body(data);
    }
}

void Help::upsert_blob(served::response &res, const served::request &req, bool is_common)
{
    uint32_t id = Helper::get_element_id(req);
    uint32_t blob_id = Helper::get_element_id(req);

    // Проверяем права на Help item
    check_help_item_perms(req, is_common, id);

    // Создаём папку если её нет
    const std::string base_path = help_item_path(id);
    const std::string file_path = base_path + std::to_string(blob_id);

    const std::string perm = Helpz::File::exist(file_path) ? "change_help" : "add_help";
    Auth_Middleware::check_permission(perm);

    if (!Helpz::File::exist(base_path))
        Helpz::File::create_dir(base_path);

    const std::string content_type = req.header("Content-Type");
    if (content_type.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Unknown content type");

    uint32_t content_type_size = content_type.size();

    Helpz::File file{file_path};
    if (!file.open(Helpz::File::WRITE_ONLY | Helpz::File::TRUNCATE))
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, file.last_error());

    file.write(reinterpret_cast<const char*>(&content_type_size), 8);
    file.write(content_type.c_str(), content_type_size);
    file.write(req.body());
    file.close();

    res.set_status(201);
}

QString Help::base_sql_where(const served::request &req, bool is_common) const
{
    if (is_common)
        return "WHERE scheme_id IS NULL ";
    const Scheme_Info scheme = Scheme::get_info(req);
    return "WHERE " + scheme.ids_to_sql() + ' ';
}

std::string Help::help_item_path(uint32_t id) const
{
    return Rest::Config::instance()._blob_dir_path + "/help/" + std::to_string(id) + '/';
}

void Help::check_help_item_perms(const served::request &req, bool is_common, uint32_t id)
{
    Helpz::DB::Base& db = Helpz::DB::Base::get_thread_local_instance();
    QString sql = base_sql_where(req, is_common) + "AND id = " + QString::number(id);
    auto q = db.select(db_table<DB::Help>(), sql, {}, {DB::Help::COL_id});
    if (!q.isActive() || !q.next())
//        throw served::request_error(served::status_4XX::BAD_REQUEST, "Unknown element id"); // TODO: if debug mode false.
        throw served::request_error(served::status_5XX::INTERNAL_SERVER_ERROR, db.last_error().toStdString());
}

} // namespace Rest
} // namespace Das
