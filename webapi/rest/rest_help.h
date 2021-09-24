#ifndef DAS_REST_HELP_H
#define DAS_REST_HELP_H

#include <served/served.hpp>

#include <plus/das/scheme_info.h>

namespace Das {
namespace Rest {

class Help
{
public:
    Help(served::multiplexer& mux, const std::string& scheme_path);
private:
    void register_handlers(served::multiplexer& mux, const std::string &base_path, bool is_common);

    void get_tree(served::response& res, const served::request& req, bool is_common);
    void upsert_tree(served::response& res, const served::request& req, bool is_common);
    void get_item(served::response& res, const served::request& req, bool is_common);
    void del_item(served::response& res, const served::request& req, bool is_common);
    void get_blob_head(served::response& res, const served::request& req, bool is_common);
    void get_blob(served::response& res, const served::request& req, bool is_common, bool is_head = false);
    void upsert_blob(served::response& res, const served::request& req, bool is_common);

    QString base_sql_where(const served::request &req, bool is_common) const;
    std::string help_item_path(uint32_t id) const;

    void check_help_item_perms(const served::request& req, bool is_common, uint32_t id);
//    void create_user_dir
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_HELP_H
