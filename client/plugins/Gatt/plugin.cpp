#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QFile>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusError>
#include <QDBusMetaType>

#include <Helpz/settingshelper.h>
#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/scheme.h>

#include "plugin.h"

namespace Das {

Q_LOGGING_CATEGORY(GattLog, "gatt")
Q_LOGGING_CATEGORY(GattLogDetail, "gatt.detail", QtInfoMsg)

static constexpr const QLatin1String _bluez("org.bluez");
// ----------

Gatt_Plugin::Gatt_Plugin()
    : QObject()
    , _conn(QString())
    , _connecting_dev(nullptr)
{
    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();

    connect(&_start_timer, &QTimer::timeout, this, &Gatt_Plugin::start);
    _start_timer.setSingleShot(true);
    _start_timer.setInterval(15000);

    connect(&_connect_timer, &QTimer::timeout, this, &Gatt_Plugin::process_connected);
    _connect_timer.setSingleShot(true);
}

Gatt_Plugin::~Gatt_Plugin()
{
    stop();
}

void Gatt_Plugin::configure(QSettings *settings)
{
    _conn = QDBusConnection::systemBus();
    if (!_conn.isConnected())
        throw std::runtime_error("Can't connect to the system D-Bus: " + _conn.lastError().message().toStdString());

    _obj_mng.reset(new QDBusInterface(_bluez, "/", "org.freedesktop.DBus.ObjectManager", _conn));
    if (!_obj_mng->isValid())
        throw std::runtime_error("Can't find org.bluez D-Bus service: " + _conn.lastError().message().toStdString());

    _conn.connect(_bluez, "/", "org.freedesktop.DBus.ObjectManager", "InterfacesAdded",
                               this, SLOT(interfaces_added(QDBusObjectPath,InterfaceList)));
//    connect(&iface, SIGNAL(InterfacesAdded(QDBusObjectPath,InterfaceList)),
//            this, SLOT(interfaces_added(QDBusObjectPath,InterfaceList)));
    connect(_obj_mng.get(), SIGNAL(InterfacesRemoved(QDBusObjectPath,QStringList)),
            this, SLOT(interfaces_removed(QDBusObjectPath,QStringList)));

    using Helpz::Param;
    _conf = Helpz::SettingsHelper(
        settings, "Gatt",
        Param<QString>{"BluetoothAdapter", "/org/bluez/hci0"},
        Param<std::chrono::seconds>{"ScanTimeoutSecs", std::chrono::seconds{4}},
        Param<std::chrono::seconds>{"ConnectTimeoutSecs", std::chrono::seconds{10}},
        Param<std::chrono::seconds>{"ReadTimeoutSecs", std::chrono::seconds{10}}
        ).obj<Config>();

    for (Device* dev : scheme()->devices())
        if (dev->checker_type()->checker == this)
            init_device(dev);
}

void Gatt_Plugin::start()
{
    if (!_adapter || !_prop_iface)
    {
        _adapter.reset(new QDBusInterface(_bluez, _conf._adapter, _bluez + ".Adapter1", _conn));
        _prop_iface.reset(new QDBusInterface(_bluez, _conf._adapter, "org.freedesktop.DBus.Properties", _conn));

        if (!_adapter->isValid() || !_prop_iface->isValid())
        {
            qCCritical(GattLog) << "Can't find bluetooth adapter in D-Bus service:" << _conn.lastError().message();
            restart();
            return;
        }

        _conn.connect(_bluez, _conf._adapter, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                      this, SLOT(props_changed(QString,QVariantMap,QStringList)));
    }

    _adapter->call("StopDiscovery");
    toggle_adapter_power(false);
    toggle_adapter_power(true);

    if (start_discovery())
        connect_next2();

    // TODO: Store address to param
    // TODO: Connect to param changed
    // _adapter->call("RemoveDevice", QDBusObjectPath()); RemoveDevice(QDBusObjectPath) - TODO: use it when user change param with address
    // TODO: Disconnect items when it needed
    // TODO: What about reconnect
    // TODO: Reset adapter power if no one device connected after some timeout
    // TODO: Reset _last_connect_time when disconnec
}

bool Gatt_Plugin::check(Device* dev)
{
    return true;
}

void Gatt_Plugin::stop()
{
}

void Gatt_Plugin::write(Device* /*dev*/, std::vector<Write_Cache_Item>& /*items*/) {}

void Gatt_Plugin::print_list_devices()
{

}

void Gatt_Plugin::find_devices(const QStringList &accepted_services, const QStringList &accepted_characteristics)
{
}

void Gatt_Plugin::props_changed(const QString& iface, const QVariantMap& changed, const QStringList& invalidated)
{
    qCDebug(GattLogDetail) << "props" << iface << "Changed:" << changed << "Invalidated:" << invalidated;
}

void Gatt_Plugin::interfaces_added(const QDBusObjectPath& object_path, const InterfaceList& interfaces)
{

}

void Gatt_Plugin::interfaces_removed(const QDBusObjectPath& object_path, const QStringList& interfaces)
{
    const QString path = object_path.path();
    if (!path.startsWith(_conf._adapter))
        return;

    if (path.size() == _conf._adapter.size()) // Adapter disconnected
    {
        restart();
        return;
    }

    for (auto&& it : _devs)
    {
        Gatt_Device_Info& info = it.second;
        if (path == info._dev_path)
        {
            std::vector<Device_Item*> disconnected;
            for (auto&& it: info._items_chars)
            {
                disconnected.push_back(it->item());
                it->stop();
            }

            QMetaObject::invokeMethod(it.first, "set_device_items_disconnect", Qt::QueuedConnection,
                Q_ARG(std::vector<Device_Item*>, disconnected));

            info._dev_path.clear();
            info._dev_iface.reset();
            info._prop_iface.reset();

            // TODO: start discovery for this device
            break;
        }
    }
    // TODO: Remove from _excluded_devs

}

void Gatt_Plugin::receive_timeout(Device_Item* item)
{


    // TODO: do something
}

ManagedObjectList Gatt_Plugin::get_managed_objects()
{
    if (!_obj_mng)
        return {};

    QDBusReply<ManagedObjectList> reply = _obj_mng->call("GetManagedObjects");
    if (!reply.isValid())
        qCCritical(GattLog) << "Call GetManagedObjects failed:" << reply.error().message();
    return reply.value();
}

bool Gatt_Plugin::start_discovery()
{
    QVariantMap discoveryFilter;
    discoveryFilter.insert("Transport", "le");
    discoveryFilter.insert("DuplicateData", false);
    auto res = _adapter->call("SetDiscoveryFilter", discoveryFilter);
    if (res.type() == QDBusMessage::ErrorMessage)
        qCWarning(GattLog) << "Fail to call SetDiscoveryFilter" << res.errorName() << res.errorMessage();

    res = _adapter->call("StartDiscovery");
    if (res.type() == QDBusMessage::ErrorMessage)
    {
        if (res.errorName() != "org.bluez.Error.InProgress")
        {
            qCCritical(GattLog) << "Fail to call StartDiscovery" << res.errorName() << res.errorMessage();
            restart();
            return false;
        }
        qCWarning(GattLog) << "Fail to call StartDiscovery" << res.errorName() << res.errorMessage();
    }
    return true;
}

void Gatt_Plugin::restart()
{
    _connect_timer.stop();
    disconnect(nullptr, nullptr, this, SLOT(props_changed(QString,QVariantMap,QStringList)));
    _excluded_devs.clear();
    _connecting_dev = nullptr;
    for (auto&& it : _devs)
    {
        it.second._dev_path.clear();
        it.second._dev_iface.reset();
        it.second._prop_iface.reset();
    }
    toggle_adapter_power(false);

    _adapter.reset();
    _prop_iface.reset();
    _start_timer.start();
}

void Gatt_Plugin::init_device(Device* dev)
{
    QString service = dev->param("service").toString();
    uint32_t param_id = dev->param("address_param_id").toUInt();
    if (service.isEmpty() || !param_id)
        return;

    Device_item_Group* dig = nullptr;

    Gatt_Device_Info info;
    for (Device_Item* item : dev->items())
    {
        QString characteristic = item->param("characteristic").toString();
        if (characteristic.isEmpty())
            continue;

        if (!dig)
            dig = item->group();

        auto receiver = std::make_shared<Gatt::Characteristic_Receiver>(item, std::move(characteristic), this);
        connect(receiver.get(), &Gatt::Characteristic_Receiver::timeout, this, &Gatt_Plugin::receive_timeout);
        info._items_chars.push_back(std::move(receiver));
    }

    if (!dig)
        return;

    info._address_param = dig->params()->get_by_id(param_id);
    if (!info._address_param)
        return;

    info._address = info._address_param->value().toString();
    info._service = std::move(service);

    _devs.emplace(dev, std::move(info));
}

void Gatt_Plugin::toggle_adapter_power(bool state)
{
    if (!_prop_iface)
        return;

    QDBusMessage res = _prop_iface->call("Set", _bluez + ".Adapter1", "Powered", QVariant::fromValue(QDBusVariant(state)));
    if (res.type() == QDBusMessage::ErrorMessage)
        qCCritical(GattLog) << "Can't toggle adapter power to" << state << res.errorName() << res.errorMessage();

    QDBusReply<QDBusVariant> reply = _prop_iface->call("Get", _bluez + ".Adapter1", "Powered");
    if (!reply.isValid())
        qCCritical(GattLog) << "Call Get adapter power state failed:" << reply.error();
    else if (reply.value().variant().toBool() != state)
        qCDebug(GattLog) << "Adapter power state not changed to" << state;
}

QString Gatt_Plugin::get_address(const InterfaceList& interfaces, QStringList* service_uuids)
{
    auto devIface = interfaces.find(_bluez + ".Device1");
    if (devIface == interfaces.cend())
        return {};

    const QVariantMap& devProps = devIface.value();
    auto adr = devProps.find("Address");
    if (adr == devProps.cend())
        return {};

    if (service_uuids)
    {
        auto uuidsIt = devProps.find("UUIDs");
        if (uuidsIt != devProps.cend())
            *service_uuids = uuidsIt.value().toStringList();
    }

    return adr.value().toString();
}

QString Gatt_Plugin::get_address(const Gatt_Device_Info& info)
{
    if (!info._prop_iface)
        return info._address;

    QDBusReply<QDBusVariant> adr = info._prop_iface->call("Get", _bluez + ".Device1", "Address");
    if (adr.isValid())
        return adr.value().variant().toString();
    return info._address;
}

std::map<Device*, Gatt_Plugin::Gatt_Device_Info>::iterator Gatt_Plugin::get_next_disconnected()
{
    for (auto it = _devs.begin(); it != _devs.end(); ++it)
    {
        Gatt_Device_Info& info = it->second;
        if (info.is_can_connection())
            return it;
    }
    return _devs.end();
}

void Gatt_Plugin::connect_next2()
{
    auto disconnectedIt = get_next_disconnected();
    if (disconnectedIt == _devs.end())
    {
        for (auto&& it : _devs)
            if (it.second.is_connected())
                return;

        // No one can connection. Restart.
        // А что если таймаут не вышел для не обработанного в этот раз устройства. (знач _start_timer должен иметь интервал не менее таймаута для переподключения)
        restart();
        return;
    }

    Gatt_Device_Info& info = disconnectedIt->second;
    info._dev_path.clear();

    QStringList service_uuids;
    const ManagedObjectList objs = get_managed_objects();
    for (auto it = objs.cbegin(); it != objs.cend(); ++it)
    {
        const QString address = get_address(it.value(), info._address.isEmpty() ? &service_uuids : nullptr);
        if (address.isEmpty())
            continue;

        if (info._address.isEmpty())
        {
            if (_excluded_devs.find(address) != _excluded_devs.cend()
                || info._excluded_devs.find(address) != info._excluded_devs.cend())
                continue;

            if (service_uuids.empty())
            {
                info._excluded_devs.insert(address);
                info._dev_path = it.key().path();
            }
            else if (service_uuids.contains(info._service, Qt::CaseInsensitive))
            {
                // TODO: Store address to param
                info._address = address;
                info._dev_path = it.key().path();
                _excluded_devs.insert(address);
            }
            else
            {
                info._excluded_devs.insert(address);
                continue;
            }
        }
        else if (info._address.compare(address, Qt::CaseInsensitive) == 0)
        {
            info._dev_path = it.key().path();
            _excluded_devs.insert(address);
        }
        break;
    }

    if (info._dev_path.isEmpty())
    {
        info._last_connect_time = clock::now();
        connect_next2();
        return;
    }

    info._dev_iface.reset(new QDBusInterface(_bluez, info._dev_path, _bluez + ".Device1", _conn));
    if (!info._dev_iface->isValid())
    {
        info._dev_iface.reset();
        if (!info._address.isEmpty())
            info._last_connect_time = clock::now();
        connect_next2();
        return;
    }

    info._prop_iface.reset(new QDBusInterface(_bluez, info._dev_path, "org.freedesktop.DBus.Properties", _conn));

    _connecting_dev = disconnectedIt->first;
    info._dev_iface->call("Connect");
    _connect_start_time = clock::now();
    _connect_timer.start(4000);
}

void Gatt_Plugin::process_connected()
{
    auto it = _devs.find(_connecting_dev);
    if (it == _devs.cend())
    {
        abort_current_and_connect_next();
        return;
    }

    Gatt_Device_Info& info = it->second;
    if (!info._dev_iface || !info._prop_iface)
    {
        abort_current_and_connect_next(&info);
        return;
    }

    QDBusReply<QDBusVariant> service_uuids_reply = info._prop_iface->call("Get", _bluez + ".Device1", "UUIDs");
    if (!service_uuids_reply.isValid())
    {
        qCCritical(GattLog) << "Call Get UUIDs failed:" << service_uuids_reply.error();
        abort_current_and_connect_next(&info);
        return;
    }

    const QStringList service_uuids = service_uuids_reply.value().variant().toStringList();
    if (service_uuids.empty())
    {
        if ((clock::now() - _connect_start_time) >= _conf._connect_timeout)
        {
            _excluded_devs.insert(info._address);
            abort_current_and_connect_next(&info);
        }
        else
            _connect_timer.start(1000);
        return;
    }

    if (!service_uuids.contains(info._service, Qt::CaseInsensitive))
    {
        qCCritical(GattLog) << "Device connected, but it's hasn't service" << info._service
                            << "for device" << it->first->toString() << "UUIDs" << service_uuids;
        abort_current_and_connect_next(&info);
        return;
    }

    const ManagedObjectList objs = get_managed_objects();
    int characteristic_count = 0;

    std::vector<std::shared_ptr<Gatt::Characteristic_Receiver>> items_chars = info._items_chars;
    for (auto it = objs.cbegin(); it != objs.cend() && !items_chars.empty(); ++it)
    {
        if (!it.key().path().startsWith(info._dev_path))
            continue;

        const InterfaceList& interfaces = it.value();
        auto itIface = interfaces.find(_bluez + ".GattCharacteristic1");
        if (itIface == interfaces.cend())
            continue;

        const QVariantMap& params = itIface.value();
        auto itUuid = params.find(QLatin1String("UUID"));
        if (itUuid == params.cend())
            continue;

        ++characteristic_count;

        const QString uuid = itUuid.value().toString();
        for (auto itemIt = items_chars.begin(); itemIt != items_chars.end(); ++itemIt)
        {
            if (uuid.compare((*itemIt)->uuid(), Qt::CaseInsensitive) == 0)
            {
                if (start_receive(*itemIt, it.key().path()))
                    items_chars.erase(itemIt);
                break;
            }
        }
    }

    if (!characteristic_count
        && (clock::now() - _connect_start_time) < _conf._connect_timeout)
    {
        _connect_timer.start(1000);
    }
    else
    {
        const QString address = get_address(info);
        if (items_chars.size() != info._items_chars.size())
        {
            if (info._address.isEmpty())
            {
                info._address = address;
                // TODO: Store address to param
            }
            info._last_connect_time = clock::time_point(clock::duration::max());

            _excluded_devs.insert(address);
            abort_current_and_connect_next(nullptr);
        }
        else
        {
            if (!characteristic_count)
                _excluded_devs.insert(address);

            abort_current_and_connect_next(&info, &address);
        }
    }
}

void Gatt_Plugin::abort_current_and_connect_next(Gatt_Device_Info* info, const QString* address)
{
    if (info)
    {
        if (info->_dev_iface)
        {
            info->_dev_iface->call("Disconnect");
            info->_dev_iface.reset();
        }

        if (info->_prop_iface)
        {
            if (info->_address.isEmpty())
            {
                const QString adr = address ? *address : get_address(*info);
                if (!adr.isEmpty())
                    info->_excluded_devs.insert(adr);
            }

            info->_prop_iface.reset();
        }
    }
    _connecting_dev = nullptr;
    connect_next2();
}

bool Gatt_Plugin::start_receive(std::shared_ptr<Gatt::Characteristic_Receiver> receiver, const QString& path)
{
    QDBusInterface chr(_bluez, path, _bluez + ".GattCharacteristic1", _conn);
    if (!chr.isValid())
    {
        qCCritical(GattLog) << "Can't get iface GattCharacteristic1" << _conn.lastError().message();
        return false;
    }

    bool res = _conn.connect(_bluez, path, "org.freedesktop.DBus.Properties", "PropertiesChanged", receiver.get(), SLOT(dev_props_changed(QString,QVariantMap,QStringList)));
    if (!res)
    {
        qCCritical(GattLog) << "Can't connect to GattCharacteristic1 PropertiesChanged" << _conn.lastError().message();
        return false;
    }

    auto msgRes = chr.call("StartNotify");
    if (msgRes.type() == QDBusMessage::ErrorMessage)
    {
        qCCritical(GattLog) << "Can't StartNotify" << msgRes.errorName() << msgRes.errorMessage();
        return false;
    }

    receiver->set_path(path);
    return true;
}

} // namespace Das
