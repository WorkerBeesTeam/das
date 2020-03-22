#include "menu_item.h"

namespace Das {
namespace Bot {

using namespace std;

Menu_Item::Menu_Item(const Bot_Base &base, uint32_t user_id, const Scheme_Item &scheme) :
    Bot_Base(base),
    skip_edit_(false),
    user_id_(user_id),
    scheme_(scheme)
{
}

void Menu_Item::create_keyboard()
{
    keyboard_.reset(new TgBot::InlineKeyboardMarkup);
}

} // namespace Bot
} // namespace Das
