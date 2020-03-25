#ifndef DAS_BOT_USER_MENU_DEVICE_ITEM_H
#define DAS_BOT_USER_MENU_DEVICE_ITEM_H

#include <QVariant>

#include "named_data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

class Device_Item_Value_Normalizer;
class Device_Item final : public Named_Data_Item
{
public:
    Device_Item();

    std::string get_value() const override;

private:
    std::string get_name() const override;

    struct Value_Pair {
        QVariant value_;
        std::string sign_;
    };

    Value_Pair get_value_pair() const;

    void parse(const picojson::object &data) override;

    std::shared_ptr<Device_Item_Value_Normalizer> normalizer_;
};

} // namespace User_Menu
} // namespace Bot
} // namespace Das

#endif // DAS_BOT_USER_MENU_DEVICE_ITEM_H
