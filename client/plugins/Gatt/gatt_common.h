#ifndef DAS_GATT_COMMON_H
#define DAS_GATT_COMMON_H

#include <string>
#include <vector>

//#include <bluetooth/sdp.h>
#include <gattlib.h>

namespace Das {

struct Gatt_Adapter {
    std::string _name;
    void* _adapter = nullptr;
};

class Gatt_Common {
public:
    static Gatt_Adapter open_adapter();
    static uuid_t string_to_uuid(const std::string& str);
    static std::string uuid_to_string(const uuid_t& uuid);
    static std::vector<uuid_t> strings_to_uuids(const std::vector<std::string>& data);
    static bool uuid_equals(const uuid_t& item1, const uuid_t& item2);
};

struct Gatt_Device_Name {
    std::string _addr, _name;
    std::string title() const;

    bool operator <(const Gatt_Device_Name& o) const;
};

struct Gatt_Device_Info {
    std::vector<uuid_t> _services;
    std::vector<uuid_t> _characteristics;
};

} // namespace Das

#endif // DAS_GATT_COMMON_H
