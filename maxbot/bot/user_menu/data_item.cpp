#include "connection_state.h"
#include "device_item.h"
#include "dig_mode.h"
#include "dig_name.h"
#include "dig_param.h"

#include "data_item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

Data_Item::Data_Item() :
    max_length_(0),
    parameters_(nullptr)
{
}

std::shared_ptr<Data_Item> Data_Item::create(const picojson::object &data, const Parameters *parameters)
{
    const std::string type = data.at("type").get<std::string>();

    std::shared_ptr<Data_Item> ptr;

    if (type == "connection_state") ptr = std::make_shared<Connection_State>();
    else if (type == "dig_name")    ptr = std::make_shared<DIG_Name>();
    else if (type == "dig_mode")    ptr = std::make_shared<DIG_Mode>();
    else if (type == "dig_param")   ptr = std::make_shared<DIG_Param>();
    else if (type == "di")          ptr = std::make_shared<Device_Item>();

    if (!ptr)
        throw std::runtime_error("Failed to create type: " + type);

    ptr->parameters_ = parameters;
    ptr->parse(data);

    return ptr;
}

std::string Data_Item::value() const
{
    std::string res = get_value();

    if (max_length_ > 0 && res.size() > max_length_)
        res.resize(max_length_);
    return res;
}

std::string Data_Item::get_value() const
{
    return "Unknown";
}

void Data_Item::parse(const picojson::object &data)
{
    auto it = data.find("max_length");
    if (it != data.cend())
        max_length_ = it->second.get<int64_t>();
}

const Parameters &Data_Item::parameters() const
{
    return *parameters_;
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
