#ifndef DAS_BOT_BOT_BASE_H
#define DAS_BOT_BOT_BASE_H

#include <tgbot/types/InlineKeyboardButton.h>

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
protected:
    TgBot::InlineKeyboardButton::Ptr makeInlineButton(const std::string& data, const std::string& text) const;
    std::vector<TgBot::InlineKeyboardButton::Ptr> makeInlineButtonRow(const std::string& data, const std::string& text) const;

    const char* default_status_category_emoji(uint32_t category_id) const;

    DBus::Interface* dbus_iface_;
};

} // namespace Bot
} // namespace Das

#endif // BOT_BASE_H
