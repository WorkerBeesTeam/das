#ifndef DAS_GATT_FINDER_H
#define DAS_GATT_FINDER_H

#include <functional>
#include <chrono>

#include "gatt_common.h"

namespace Das {

class Gatt_Finder {
public:

    std::vector<Gatt_Device_Name> find_devices(const std::vector<std::string>& accepted_services = {},
        const std::vector<std::string>& accepted_characteristics = {}, std::chrono::seconds timeout = std::chrono::seconds{4});

    void print_all_device_info(std::function<void (const std::string &)> log_cb, std::chrono::seconds timeout = std::chrono::seconds{4});
    std::vector<Gatt_Device_Name> get_devices(std::chrono::seconds timeout = std::chrono::seconds{4}) const;
    Gatt_Device_Info get_device_info(const std::string& addr);

    bool check_includes(const std::vector<uuid_t>& v1, const std::vector<uuid_t>& v2) const;

private:

    static void ble_discovered_device(void */*adapter*/, const char* addr, const char* name, void *user_data);
    std::vector<uuid_t> parse_services(gatt_connection_t* connection);
    std::vector<uuid_t> parse_characteristics(gatt_connection_t* connection);
};

} // namespace Das

#endif // DAS_GATT_FINDER_H
