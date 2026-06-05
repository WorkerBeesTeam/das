#include <algorithm>
#include <maxbot/types/Keyboard.h>
#include <memory>

#include "bot_base.h"

namespace Das {
namespace Bot {

using namespace std;

Bot_Base::Bot_Base(DBus::Interface *dbus_iface) :
    dbus_iface_(dbus_iface)
{ }

/*static*/ string Bot_Base::prepare_str(string text)
{
    int x = count(text.cbegin(), text.cend(), '*');
    if (x % 2 != 0) text += '*';

    x = count(text.cbegin(), text.cend(), '_');
    if (x % 2 != 0) text += '_';

    x = count(text.cbegin(), text.cend(), '`');
    if (x % 2 != 0) text += '`';
    return text;
}

/*static*/ MaxBot::Button::Ptr Bot_Base::makeInlineButton(const string &data, const string &text)
{
	auto cb = std::make_shared<MaxBot::CallbackButton>();
	cb->payload = data;

    auto button = std::make_shared<MaxBot::Button>();
    button->text = text;
    button->type = "callback";
    button->_data = std::move(cb);
    return button;
}

/*static*/ std::vector<MaxBot::Button::Ptr> Bot_Base::makeInlineButtonRow(const string &data, const string &text)
{
    return { makeInlineButton(data, text) };
}

/*static*/ const char *Bot_Base::default_status_category_emoji(uint32_t category_id)
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
