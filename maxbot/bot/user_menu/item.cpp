#include <fstream>

#include <boost/algorithm/string/replace.hpp>

#include "bot/scheme_item.h"
#include "item.h"

namespace Das {
namespace Bot {
namespace User_Menu {

Item::Item(const std::string &template_path, DBus::Interface *dbus_iface) :
    parameters_{0, nullptr, dbus_iface}
{
    parse_template(read_template(template_path));
}

std::string Item::name() const
{
    return name_;
}

std::string Item::get_text(uint32_t user_id, const Scheme_Item& scheme) const
{
    parameters_.user_id_ = user_id;
    parameters_.scheme_ = &scheme;
    std::string text = '*' + scheme.title_.toStdString() + "*\n";

    std::string template_text = template_;
    for (auto& it: data_)
        boost::replace_all(template_text, "${" + it.first + '}', it.second->value());

    parameters_.scheme_ = nullptr;

    text += template_text;
    return text;
}

bool Item::operator<(const Item &o) const
{
    return order_ < o.order_;
}

picojson::object Item::read_template(const std::string &template_path)
{
    std::string template_raw;

    std::ifstream is{template_path, std::ios::binary | std::ios::ate};
    if (is)
    {
        std::size_t size = is.tellg();
        template_raw.resize(size);
        is.seekg(0);
        if(!is.read(template_raw.data(), size))
            throw std::runtime_error(strerror(errno));
    }
    else
        throw std::runtime_error(strerror(errno));

    picojson::value val;
    const std::string err = picojson::parse(val, template_raw);
    if (!err.empty() || !val.is<picojson::object>())
        throw std::runtime_error(err.empty() ? "Object is expected" : err);

    return val.get<picojson::object>();
}

void Item::parse_template(const picojson::object& obj)
{
    name_ = obj.at("name").get<std::string>();
    template_ = obj.at("template").get<std::string>();

    auto order_it = obj.find("order");
    if (order_it == obj.cend())
    {
        static int s_order = 0;
        order_ = s_order++;
    }
    else
        order_ = order_it->second.get<int64_t>();

    auto data_it = obj.find("data");
    if (data_it == obj.cend() || !data_it->second.is<picojson::object>())
        return;

    const picojson::object data = data_it->second.get<picojson::object>();
    for (auto& it: data)
    {
        if (it.second.is<picojson::object>())
            data_.emplace(it.first, Data_Item::create(it.second.get<picojson::object>(), &parameters_));
    }
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
