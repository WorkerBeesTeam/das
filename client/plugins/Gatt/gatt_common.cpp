#include <stdexcept>
#include <cstring>

#include "gatt_common.h"

namespace Das {

Gatt_Adapter Gatt_Common::open_adapter()
{
    Gatt_Adapter adapter;
    const char* adapter_name = nullptr;
    int ret = gattlib_adapter_open(adapter_name, &adapter._adapter);
    if (ret)
        throw std::runtime_error("Failed to open bluetooth adapter. Error code: " + std::to_string(ret));
    if (adapter_name)
        adapter._name = adapter_name;
    return adapter;
}

uuid_t Gatt_Common::string_to_uuid(const std::string &str)
{
    uuid_t uuid;
    if (gattlib_string_to_uuid(str.c_str(), str.size(), &uuid) == GATTLIB_SUCCESS)
        return uuid;
    return {0, 0};
}

std::string Gatt_Common::uuid_to_string(const uuid_t &uuid)
{
    char uuid_str[MAX_LEN_UUID_STR + 1];
    gattlib_uuid_to_string(&uuid, uuid_str, sizeof(uuid_str));
    return std::string{uuid_str};
}

std::vector<uuid_t> Gatt_Common::strings_to_uuids(const std::vector<std::string> &data)
{
    uuid_t uuid;
    std::vector<uuid_t> res;
    for (const std::string& item: data)
        if (gattlib_string_to_uuid(item.c_str(), item.size(), &uuid) == GATTLIB_SUCCESS)
            res.push_back(uuid);
    return res;
}

bool Gatt_Common::uuid_equals(const uuid_t &item1, const uuid_t &item2)
{
    if (item1.type == item2.type) {
        switch (item1.type) {
        case SDP_UUID16: return item1.value.uuid16 == item2.value.uuid16;
        case SDP_UUID32: return item1.value.uuid32 == item2.value.uuid32;
        case SDP_UUID128: return std::memcmp(&item1.value.uuid128, &item2.value.uuid128, sizeof(uint128_t)) == 0;
        default: break;
        }
    }

    return false;
}

// Gatt_Device_Name --

std::string Gatt_Device_Name::title() const
{
    std::string title = _addr;
    if (!_name.empty())
        title += " - " + _name;
    return title;
}

bool Gatt_Device_Name::operator <(const Gatt_Device_Name &o) const
{
    return _addr < o._addr;
}

} // namespace Das
