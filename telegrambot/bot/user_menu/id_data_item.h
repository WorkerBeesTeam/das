#ifndef DAS_BOT_USER_MENU_ID_DATA_ITEM_H
#define DAS_BOT_USER_MENU_ID_DATA_ITEM_H

#include "data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class ID_Data_Item : public Data_Item
{
public:
    ID_Data_Item();

    uint32_t id() const;

protected:
    virtual void parse(const picojson::object &data) override;

private:
    uint32_t id_;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_ID_DATA_ITEM_H
