#ifndef DAS_BOT_BOT_BASE_H
#define DAS_BOT_BOT_BASE_H

#include <maxbot/types/Keyboard.h>

namespace Das {

namespace DBus {
class Interface;
} // namespace DBus

namespace Bot {

class Bot_Base
{
public:
    Bot_Base(DBus::Interface* dbus_iface);
    Bot_Base(const Bot_Base&) = default;

    static std::string prepare_str(std::string text);

    static MaxBot::Button::Ptr makeInlineButton(const std::string& data, const std::string& text);
    static std::vector<MaxBot::Button::Ptr> makeInlineButtonRow(const std::string& data, const std::string& text);

protected:
    static const char* default_status_category_emoji(uint32_t category_id);

    DBus::Interface* dbus_iface_;
};

} // namespace Bot
} // namespace Das

#endif // BOT_BASE_H
