#ifndef DAS_BOT_USER_MENU_CONNECTION_STATE_H
#define DAS_BOT_USER_MENU_CONNECTION_STATE_H

#include "data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class Connection_State final : public Data_Item
{
public:
    Connection_State();

    std::string get_value() const override;

    static std::string to_string(uint8_t state);
    static std::string get_emoji(uint8_t state);
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_CONNECTION_STATE_H
