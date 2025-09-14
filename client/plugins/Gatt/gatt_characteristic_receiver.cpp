#include "gatt_characteristic_receiver.h"

#include <Das/device.h>

namespace Das {
namespace Gatt {

Characteristic_Receiver::Characteristic_Receiver(Device_Item* item, QString&& characteristic, QObject *parent)
    : QObject{parent}
    , _item(item)
    , _uuid(std::move(characteristic))
{
    connect(&_timer, &QTimer::timeout, this, &Characteristic_Receiver::emit_timeout);
    _timer.setSingleShot(true);
    _timer.setInterval(60000);
}

Characteristic_Receiver::~Characteristic_Receiver()
{
    _timer.stop();
}

Device_Item* Characteristic_Receiver::item() const
{
    return _item;
}

const QString& Characteristic_Receiver::uuid() const
{
    return _uuid;
}

const QString& Characteristic_Receiver::path() const
{
    return _path;
}

void Characteristic_Receiver::set_path(const QString& path)
{
    _path = path;
    _timer.start();
}

void Characteristic_Receiver::stop()
{
    _timer.stop();
}

void Characteristic_Receiver::dev_props_changed(const QString&, const QVariantMap& changed, const QStringList&)
{
    auto it = changed.find("Value");
    if (it == changed.cend())
        return;

    const QByteArray value = it.value().toByteArray();

    const std::map<Device_Item*, Device::Data_Item> values{
        {_item, Device::Data_Item{0, DB::Log_Base_Item::current_timestamp(), value.toHex()}}
    };

    QMetaObject::invokeMethod(_item->device(), "set_device_items_values", Qt::QueuedConnection,
        QArgument<std::map<Device_Item*,Device::Data_Item>>("std::map<Device_Item*,Device::Data_Item>", values),
        Q_ARG(bool, true));

    _timer.start();
}

void Characteristic_Receiver::emit_timeout()
{
    emit timeout(_item);
}

} // namespace Gatt
} // namespace Das
