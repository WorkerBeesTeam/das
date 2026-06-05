#ifndef DAS_BOT_USER_MENU_DIG_MODE_H
#define DAS_BOT_USER_MENU_DIG_MODE_H

#include "id_data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class DIG_Mode final : public ID_Data_Item
{
public:
    std::string get_value() const override;

    static uint32_t get_mode_id(uint32_t scheme_id, uint32_t dig_id);
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_DIG_MODE_H
