#include "bot_base.h"

namespace Das {
namespace Bot {

using namespace std;

Bot_Base::Bot_Base(DBus::Interface *dbus_iface) :
    dbus_iface_(dbus_iface)
{

}

TgBot::InlineKeyboardButton::Ptr Bot_Base::makeInlineButton(const string &data, const string &text) const
{
    TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
    button->text = text;
    button->callbackData = data;
    return button;
}

std::vector<TgBot::InlineKeyboardButton::Ptr > Bot_Base::makeInlineButtonRow(const string &data, const string &text) const
{
    return vector<TgBot::InlineKeyboardButton::Ptr>
    {
        makeInlineButton(data, text)
    };
}

const char *Bot_Base::default_status_category_emoji(uint32_t category_id) const
{
    switch (category_id)
    {
    case 2: return "✅"; break;
    case 3: return "⚠️"; break;
    case 4: return "🚨"; break;
    default:
        break;
    }
    return "";
}

} // namespace Bot
} // namespace Das
