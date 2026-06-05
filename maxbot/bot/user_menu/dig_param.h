#ifndef DAS_BOT_USER_MENU_DIG_PARAM_H
#define DAS_BOT_USER_MENU_DIG_PARAM_H

#include "named_data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class DIG_Param final : public Named_Data_Item
{
public:
    std::string get_value() const override;
private:
    std::string get_name() const override;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_DIG_PARAM_H
