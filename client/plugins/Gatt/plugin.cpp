#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <QDebug>
#include <QSettings>
#include <QFile>

#include <Helpz/settingshelper.h>
#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/scheme.h>

#include "gatt_finder.h"
#include "gatt_notification_listner.h"
#include "plugin.h"

#define   HTU21D_I2C_ADDR 0x40

#define   HTU21D_TEMP     0xF3
#define   HTU21D_HUMID    0xF5

namespace Das {

Q_LOGGING_CATEGORY(GattLog, "gatt")

// ----------

Gatt_Plugin::Gatt_Plugin() :
    QObject()
{
}

Gatt_Plugin::~Gatt_Plugin()
{
    stop();
}

void Gatt_Plugin::configure(QSettings *settings)
{
    using Helpz::Param;
    _conf = Helpz::SettingsHelper(
        settings, "Gatt",
        Param<std::chrono::seconds>{"ScanTimeoutSecs", std::chrono::seconds{4}},
        Param<std::chrono::seconds>{"ReadTimeoutSecs", std::chrono::seconds{10}}
        ).obj<Config>();

    _break = false;
    _thread = std::thread(&Gatt_Plugin::run, this);
}

bool Gatt_Plugin::check(Device* dev)
{
    std::unique_lock lock(_mutex);
    auto it = std::find(_data.cbegin(), _data.cend(), dev);
    if (it != _data.cend())
        return true;

    std::vector<Device_Item*> device_items_disconnected;

    Data_Item data;
    data._dev = dev;
    data._dev_address = dev->param("address").toByteArray().trimmed();

    for (Device_Item* item: dev->items())
        if (data._dev_address.isEmpty() || !add(data, item))
            device_items_disconnected.push_back(item);

    if (!data._items.empty())
    {
        _data.push_back(std::move(data));
        lock.unlock();
        _cond.notify_one();
    }
    else
        lock.unlock();

    if (!device_items_disconnected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconnected));
    }

    return true;
}

void Gatt_Plugin::stop()
{
    std::shared_ptr<Gatt_Notification_Listner> listner = _notify_listner_ptr;
    if (listner)
        listner->stop();

    {
        std::lock_guard lock(_mutex);
        _break = true;
    }
    _cond.notify_one();

    if (_thread.joinable())
        _thread.join();
}

void Gatt_Plugin::write(Device* /*dev*/, std::vector<Write_Cache_Item>& /*items*/) {}

void Gatt_Plugin::print_list_devices()
{
    std::lock_guard lock(_adapter_mutex);
    try {
        Gatt_Finder finder;
        finder.print_all_device_info([](const std::string& text) { qInfo(GattLog) << QString::fromStdString(text); }, _conf._scan_timeout);
    } catch (const std::exception& e) {
        qWarning(GattLog) << "Exception:" << e.what();
    }
}

void Gatt_Plugin::find_devices(const QStringList &accepted_services, const QStringList &accepted_characteristics)
{
    std::lock_guard lock(_adapter_mutex);
    try {
        std::vector<std::string> srvs, chars;
        for (const QString& srv: accepted_services) srvs.push_back(srv.toStdString());
        for (const QString& chr: accepted_characteristics) chars.push_back(chr.toStdString());

        Gatt_Finder finder;
        finder.find_devices(srvs, chars, _conf._scan_timeout);
    } catch (const std::exception& e) {
        qWarning(GattLog) << "Exception:" << e.what();
    }
}

bool Gatt_Plugin::add(Data_Item &data, Device_Item *item)
{
    QByteArray characteristic = item->param("characteristic").toByteArray().trimmed();

    if (!characteristic.isEmpty())
    {
        uuid_t uuid = Gatt_Common::string_to_uuid(characteristic.toStdString());

        if (uuid.type != 0)
        {
            data._characteristics.push_back(uuid);
            data._items.emplace(item, std::move(uuid));
            return true;
        }
    }
    return false;
}

void Gatt_Plugin::run()
{
    std::unique_lock lock(_mutex, std::defer_lock);

    while (true)
    {
        lock.lock();
        _cond.wait(lock, [this](){ return _break || !_data.empty(); });

        if (_break) break;

        Data_Item data = std::move(_data.front());
        _data.pop_front();
        lock.unlock();

        read_data_item(data);
    }
}

void Gatt_Plugin::read_data_item(const Data_Item &info)
{
    std::map<Device_Item*, Device::Data_Item> values;

    std::unique_lock lock(_adapter_mutex);

    try {
        _notify_listner_ptr = std::make_shared<Gatt_Notification_Listner>(info._dev_address.toStdString(), _conf._scan_timeout);

        Gatt_Notification_Listner& listner = *_notify_listner_ptr;
        listner.set_callback([&info, &values](const uuid_t& uuid, const std::vector<uint8_t>& data_value) -> bool {
            auto it = std::find_if(info._items.cbegin(), info._items.cend(), [&uuid](const std::pair<Device_Item*, uuid_t>& item){
                return Gatt_Common::uuid_equals(item.second, uuid);
            });

            if (it != info._items.cend())
            {
                QByteArray value{reinterpret_cast<const char*>(data_value.data()), data_value.size()};
                Device::Data_Item data_item{0, DB::Log_Base_Item::current_timestamp(), value.toHex()};

                auto v_it = values.find(it->first);
                if (v_it == values.end())
                    values.emplace(it->first, std::move(data_item));
                else
                    v_it->second = std::move(data_item);
            }
            return info._items.size() == values.size();
        });
        listner.start(info._characteristics);
        listner.exec(_conf._read_timeout);
        _last_is_ok = true;
    } catch (const std::exception& e) {
        if (_last_is_ok)
        {
            qCCritical(GattLog) << "Exception:" << info._dev->toString() << "error:" << e.what();
            _last_is_ok = false;
        }
    }

    _notify_listner_ptr = nullptr;

    lock.unlock();

    std::vector<Device_Item*> disconnected;
    for (auto&& it: info._items)
        if (values.find(it.first) == values.cend())
            disconnected.push_back(it.first);

    if (!disconnected.empty())
        QMetaObject::invokeMethod(info._dev, "set_device_items_disconnect", Qt::QueuedConnection,
            Q_ARG(std::vector<Device_Item*>, disconnected));

    if (!values.empty())
        QMetaObject::invokeMethod(info._dev, "set_device_items_values", Qt::QueuedConnection,
            QArgument<std::map<Device_Item*, Device::Data_Item>>("std::map<Device_Item*, Device::Data_Item>", values),
            Q_ARG(bool, true));
}

} // namespace Das
