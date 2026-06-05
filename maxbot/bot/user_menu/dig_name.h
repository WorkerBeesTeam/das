#ifndef DAS_BOT_USER_MENU_DIG_NAME_H
#define DAS_BOT_USER_MENU_DIG_NAME_H

#include "id_data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class DIG_Name final : public ID_Data_Item
{
public:
    std::string get_value() const override;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_DIG_NAME_H
