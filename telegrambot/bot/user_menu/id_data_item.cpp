#include "id_data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

ID_Data_Item::ID_Data_Item()
{

}

uint32_t ID_Data_Item::id() const
{
    return id_;
}

void ID_Data_Item::parse(const picojson::object &data)
{
    Data_Item::parse(data);

    id_ = data.at("id").get<int64_t>();
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
