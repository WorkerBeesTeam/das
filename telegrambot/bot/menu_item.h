#ifndef DAS_BOT_BASE_H
#define DAS_BOT_BASE_H

#include <tgbot/types/InlineKeyboardMarkup.h>

#include "bot_base.h"
#include "scheme_item.h"

namespace Das {
namespace Bot {

class Menu_Item : public Bot_Base
{
public:
    Menu_Item(const Bot_Base& base, uint32_t user_id, const Scheme_Item &scheme);
    Menu_Item(const Menu_Item&) = default;

    bool skip_edit_;
    std::string text_;
    TgBot::InlineKeyboardMarkup::Ptr keyboard_;
protected:

    void create_keyboard();

    uint32_t user_id_;

    const Scheme_Item scheme_;
};

} // namespace Bot
} // namespace Das

#endif // DAS_BOT_BASE_H
