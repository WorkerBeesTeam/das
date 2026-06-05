#ifndef DAS_BOT_USER_MENU_DATA_ITEM_H
#define DAS_BOT_USER_MENU_DATA_ITEM_H

#include <memory>

#define PICOJSON_USE_INT64
#include <plus/jwt-cpp/include/jwt-cpp/picojson.h>

namespace Das {

namespace DBus {
class Interface;
} // namespace DBus

namespace Bot {

struct Scheme_Item;

namespace User_Menu {

struct Parameters {
    uint32_t user_id_;
    const Scheme_Item* scheme_;
    DBus::Interface* dbus_iface_;
};

class Data_Item
{
public:
    Data_Item();

    static std::shared_ptr<Data_Item> create(const picojson::object& data, const Parameters *parameters);

    std::string value() const;

protected:
    virtual std::string get_value() const;
    virtual void parse(const picojson::object& data);

    const Parameters& parameters() const;

private:
    std::size_t max_length_;
    const Parameters* parameters_;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_DATA_ITEM_H
