#include <stdexcept>

#include "gatt_finder.h"

namespace Das {

std::vector<Gatt_Device_Name> Gatt_Finder::find_devices(const std::vector<std::string> &accepted_services,
    const std::vector<std::string> &accepted_characteristics, std::chrono::seconds timeout)
{
    std::vector<uuid_t> ac_srvs  = Gatt_Common::strings_to_uuids(accepted_services);
    std::vector<uuid_t> ac_chars = Gatt_Common::strings_to_uuids(accepted_characteristics);

    std::vector<Gatt_Device_Name> res;
    std::vector<Gatt_Device_Name> devs = get_devices(timeout);

    for (auto it = devs.begin(); it != devs.end(); ++it) {
        Gatt_Device_Name& dev = *it;
        try {
            Gatt_Device_Info info = get_device_info(dev._addr);

            if (!ac_srvs.empty() && !check_includes(info._services, ac_srvs))
                continue;
            if (!ac_chars.empty() && !check_includes(info._characteristics, ac_chars))
                continue;

            res.push_back(std::move(dev));
        } catch (...) {}
    }

    return res;
}

void Gatt_Finder::print_all_device_info(std::function<void(const std::string&)> log_cb, std::chrono::seconds timeout)
{
    std::vector<Gatt_Device_Name> devs = get_devices(timeout);

    for (const Gatt_Device_Name& dev: devs)
    {
        log_cb("Discovered " + dev.title());
        try {
            Gatt_Device_Info info = get_device_info(dev._addr);

            log_cb("Services:");
            for (const uuid_t& it: info._services)
                log_cb(Gatt_Common::uuid_to_string(it));

            log_cb("Characteristics:");
            for (const uuid_t& it: info._characteristics)
                log_cb(Gatt_Common::uuid_to_string(it));

        } catch (const std::exception& e) {
            log_cb("Dev " + dev.title() + " error: " + e.what());
        }
    }
}

std::vector<Gatt_Device_Name> Gatt_Finder::get_devices(std::chrono::seconds timeout) const
{
    Gatt_Adapter adapter = Gatt_Common::open_adapter();

    std::vector<Gatt_Device_Name> devs;
    int ret = gattlib_adapter_scan_enable(adapter._adapter, &Gatt_Finder::ble_discovered_device, timeout.count(), &devs);
    if (ret)
        throw std::runtime_error("Failed to scan bluetooth devices. Error code: " + std::to_string(ret));
    gattlib_adapter_scan_disable(adapter._adapter);
    gattlib_adapter_close(adapter._adapter);
    return devs;
}

Gatt_Device_Info Gatt_Finder::get_device_info(const std::string &addr)
{
    Gatt_Adapter adapter = Gatt_Common::open_adapter();
    gatt_connection_t* connection = gattlib_connect(adapter._adapter, addr.c_str(), GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
    if (connection == nullptr) {
        gattlib_adapter_close(adapter._adapter);
        throw std::runtime_error("Fail to connect to the bluetooth device.");
    }

    Gatt_Device_Info info;

    try {
        info._services = parse_services(connection);
        info._characteristics = parse_characteristics(connection);
    } catch (...) {
        gattlib_disconnect(connection);
        throw;
    }

    gattlib_disconnect(connection);
    return info;
}

bool Gatt_Finder::check_includes(const std::vector<uuid_t> &v1, const std::vector<uuid_t> &v2) const
{
    for (const uuid_t& item1: v1)
        for (const uuid_t& item2: v2)
            if (Gatt_Common::uuid_equals(item1, item2))
                return true;
    return false;
}

void Gatt_Finder::ble_discovered_device(void *, const char *addr, const char *name, void *user_data)
{
    std::vector<Gatt_Device_Name>* data = static_cast<std::vector<Gatt_Device_Name>*>(user_data);
    if (addr)
        data->emplace_back(Gatt_Device_Name{addr, name ? name : std::string{}});
}

std::vector<uuid_t> Gatt_Finder::parse_services(gatt_connection_t *connection)
{
    std::vector<uuid_t> res;

    int services_count = 0;
    gattlib_primary_service_t* services;
    int ret = gattlib_discover_primary(connection, &services, &services_count);
    if (ret != GATTLIB_SUCCESS)
        throw std::runtime_error("Fail to discover primary services.");

    for (int i = 0; i < services_count; i++)
        res.push_back(services[i].uuid);
    free(services);
    return res;
}

std::vector<uuid_t> Gatt_Finder::parse_characteristics(gatt_connection_t *connection)
{
    std::vector<uuid_t> res;

    int characteristics_count = 0;
    gattlib_characteristic_t* characteristics;
    int ret = gattlib_discover_char(connection, &characteristics, &characteristics_count);
    if (ret != GATTLIB_SUCCESS)
        throw std::runtime_error("Fail to discover characteristics.");

    for (int i = 0; i < characteristics_count; i++)
        res.push_back(characteristics[i].uuid);
    free(characteristics);
    return res;
}

} // namespace Das
