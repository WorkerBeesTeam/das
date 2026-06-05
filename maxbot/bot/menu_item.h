#pragma once

#include <maxbot/types/NewMessageBody.h>

#include "bot_base.h"
#include "scheme_item.h"

namespace Das::Bot {

class Menu_Item : public Bot_Base
{
public:
    Menu_Item(const Bot_Base& base, uint32_t user_id, const Scheme_Item &scheme);
    Menu_Item(const Menu_Item&) = default;

    bool skip_edit_;
	MaxBot::NewMessageBody::Ptr _msg;

protected:

    void createMessage();

    uint32_t user_id_;

    const Scheme_Item scheme_;
	MaxBot::InlineKeyboardAttachmentRequest::Ptr _keyboard;
};

} // namespace Das::Bot
