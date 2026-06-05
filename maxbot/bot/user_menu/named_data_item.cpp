#include "named_data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

Named_Data_Item::Named_Data_Item() :
    is_with_name_(false)
{
}

bool Named_Data_Item::is_with_name() const { return is_with_name_; }

void Named_Data_Item::parse(const picojson::object &data)
{
    ID_Data_Item::parse(data);

    auto it = data.find("with_name");
    if (it != data.cend())
        is_with_name_ = it->second.get<bool>();
}

std::string Named_Data_Item::get_value() const
{
    std::string result;

    if (is_with_name())
    {
        result = get_name();
        result += ": ";
    }
    return result;
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
