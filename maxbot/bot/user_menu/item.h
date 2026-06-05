#ifndef DAS_BOT_USER_MENU_ITEM_H
#define DAS_BOT_USER_MENU_ITEM_H

#include <string>
#include <map>

#include "data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class Item final
{
public:
    Item(const std::string& template_path, DBus::Interface* dbus_iface);

    std::string name() const;

    std::string get_text(uint32_t user_id, const Scheme_Item &scheme) const;

    bool operator<(const Item& o) const;
private:
    picojson::object read_template(const std::string& template_path);
    void parse_template(const picojson::object& obj);

    int order_;

    std::string name_, template_;

    std::map<std::string, std::shared_ptr<Data_Item>> data_;

    mutable Parameters parameters_;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_ITEM_H
