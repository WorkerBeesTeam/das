#ifndef DAS_GATT_NOTIFICATION_LISTNER_H
#define DAS_GATT_NOTIFICATION_LISTNER_H

#include <functional>
#include <chrono>

#include "gatt_common.h"

namespace Das {

class Gatt_Notification_Listner {
public:
    Gatt_Notification_Listner(const std::string& dev_addr, const std::chrono::seconds& scan_timeout = std::chrono::seconds{4});

    ~Gatt_Notification_Listner();

    void set_callback(std::function<bool(const uuid_t&, const std::vector<uint8_t>)> cb);

    void start(const std::vector<std::string>& characteristics);
    void start(const std::vector<uuid_t>& characteristics);

    bool exec(const std::chrono::seconds &timeout = std::chrono::seconds{10});

    void stop();

private:

    static void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data);

    bool check_device_available(void* adapter, const std::chrono::seconds& scan_timeout);
    static void ble_discovered_device(void *adapter, const char* addr, const char* name, void *user_data);

    std::string _dev_addr;
    gatt_connection_t* _connection;
    std::vector<uuid_t> _characteristics;
    std::function<bool(const uuid_t&, const std::vector<uint8_t>)> _cb;
};

} // namespace Das

#endif // DAS_GATT_NOTIFICATION_LISTNER_H
