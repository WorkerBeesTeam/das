#ifndef DAS_BOT_USER_MENU_NAMED_DATA_ITEM_H
#define DAS_BOT_USER_MENU_NAMED_DATA_ITEM_H

#include "id_data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class Named_Data_Item : public ID_Data_Item
{
public:
    Named_Data_Item();

    bool is_with_name() const;

protected:
    virtual void parse(const picojson::object &data) override;

    virtual std::string get_value() const override;
    virtual std::string get_name() const = 0;
private:
    bool is_with_name_;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_NAMED_DATA_ITEM_H
