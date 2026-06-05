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

void Menu_Item::createMessage()
{
	_keyboard = std::make_shared<MaxBot::InlineKeyboardAttachmentRequest>();
	auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
	attachment->_data = _keyboard;

	_msg = std::make_shared<MaxBot::NewMessageBody>();
	_msg->format = "markdown";
	_msg->attachments.emplace_back(std::move(attachment));
}

} // namespace Bot
} // namespace Das
