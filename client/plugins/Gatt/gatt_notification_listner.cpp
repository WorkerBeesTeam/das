#include <mutex>
#include <condition_variable>

#include <stdexcept>
#include <cstring>

#include "gatt_notification_listner.h"

namespace Das {

std::mutex _m;
std::condition_variable _cond;

Gatt_Notification_Listner::Gatt_Notification_Listner(const std::string &dev_addr, const std::chrono::seconds &scan_timeout) : _dev_addr{dev_addr}
{
    Gatt_Adapter adapter = Gatt_Common::open_adapter();

    if (!check_device_available(adapter._adapter, scan_timeout))
    {
        gattlib_adapter_close(adapter._adapter);
        throw std::runtime_error("Can't find device: " + _dev_addr);
    }

    _connection = gattlib_connect(adapter._adapter, _dev_addr.c_str(), GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
    if (_connection == nullptr) {
        gattlib_adapter_close(adapter._adapter);
        throw std::runtime_error("Fail to connect to the bluetooth device: " + _dev_addr);
    }

    gattlib_register_notification(_connection, &Gatt_Notification_Listner::notification_handler, this);
}

Gatt_Notification_Listner::~Gatt_Notification_Listner()
{
    stop();
    gattlib_disconnect(_connection);
}

void Gatt_Notification_Listner::set_callback(std::function<bool (const uuid_t &, const std::vector<uint8_t>)> cb) { _cb = cb; }

void Gatt_Notification_Listner::start(const std::vector<std::string> &characteristics)
{
    start(Gatt_Common::strings_to_uuids(characteristics));
}

void Gatt_Notification_Listner::start(const std::vector<uuid_t> &characteristics)
{
    int err;
    for (const uuid_t& it: characteristics)
    {
        err = gattlib_notification_start(_connection, &it);
        if (err == GATTLIB_SUCCESS)
            _characteristics.push_back(it);
        else
            throw std::runtime_error("Fail to start notification for device: " + _dev_addr
                + " and uuid " + Gatt_Common::uuid_to_string(it)
                + " with error: " + std::to_string(err));
    }
}

bool Gatt_Notification_Listner::exec(const std::chrono::seconds& timeout)
{
    std::unique_lock lock(_m);
    return _cond.wait_for(lock, timeout) == std::cv_status::no_timeout;
}

void Gatt_Notification_Listner::stop()
{
    for (const uuid_t& notify_uuid: _characteristics)
        gattlib_notification_stop(_connection, &notify_uuid);
    _characteristics.clear();

    _cond.notify_one();
}

void Gatt_Notification_Listner::notification_handler(const uuid_t *uuid, const uint8_t *data, size_t data_length, void *user_data)
{
    auto self = static_cast<Gatt_Notification_Listner*>(user_data);
    std::vector<uint8_t> d;
    d.resize(data_length);
    std::memcpy(d.data(), data, data_length);
    if (self->_cb && !self->_cb(*uuid, d))
        self->stop();
}

bool Gatt_Notification_Listner::check_device_available(void *adapter, const std::chrono::seconds& scan_timeout)
{
    std::string dev_addr = _dev_addr;
    int ret = gattlib_adapter_scan_enable(adapter, &Gatt_Notification_Listner::ble_discovered_device, scan_timeout.count(), &dev_addr);
    if (ret)
        return false;

    if (dev_addr.empty()) // if finded
        return true;

    gattlib_adapter_scan_disable(adapter);
    return false;
}

void Gatt_Notification_Listner::ble_discovered_device(void *adapter, const char *addr, const char */*name*/, void *user_data)
{
    std::string* dev_addr = static_cast<std::string*>(user_data);
    if (addr && *dev_addr == addr)
    {
        dev_addr->clear();
        gattlib_adapter_scan_disable(adapter);
    }
}

} // namespace Das
