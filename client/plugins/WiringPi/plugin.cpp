#include <QDebug>
#include <QSettings>
#include <QFile>

#ifndef NO_WIRINGPI
#include <wiringPi.h>
#endif

#include <Helpz/settingshelper.h>

#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/scheme.h>

#include "plugin.h"

namespace Das {
namespace WiringPi {

Q_LOGGING_CATEGORY(WiringPiLog, "WiringPi")

// ----------

WiringPiPlugin::WiringPiPlugin() :
    QObject()
{
}

WiringPiPlugin::~WiringPiPlugin()
{
}

void WiringPiPlugin::configure(QSettings *settings)
{
    /*
    using Helpz::Param;
    conf = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Param<QString>,Param<QSerialPort::BaudRate>,Param<QSerialPort::DataBits>,
                            Param<QSerialPort::Parity>,Param<QSerialPort::StopBits>,Param<QSerialPort::FlowControl>,Param<int>,Param<int>,Param<int>>
        #endif
            (
                settings, "WiringPi",
                Param<QString>{"Port", "ttyUSB0"},
    ).unique_ptr<Conf>();
    */

#ifndef NO_WIRINGPI
    wiringPiSetup();

    QString mode, pud;
    int pin;
    bool ok;

    for (auto * device : scheme()->devices())
    {
        for (Device_Item * item: device->items())
        {
            pin = item->param("pin").toInt(&ok);
            if (!ok)
                continue;

            mode = item->param("mode").toString().trimmed().toLower();
            if (mode.isEmpty())
                continue;

            if (mode == "in")
                pinMode(pin, INPUT);
            else if (mode == "out")
                pinMode(pin, OUTPUT);

            pud = item->param("pud").toString().trimmed().toLower();

            if (pud == "up")
                pullUpDnControl(pin, PUD_UP);
            else if (pud == "down")
                pullUpDnControl(pin, PUD_DOWN);
            else
                pullUpDnControl(pin, PUD_OFF);
        }
    }
#endif
}

bool WiringPiPlugin::check(Device* dev)
{
    std::map<Device_Item*, Device::Data_Item> device_items_values;
    std::vector<Device_Item*> device_items_disconected;

    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    bool ok, state;
    int pin;
    QString mode;
    for (Device_Item * item: dev->items())
    {
        pin = item->param("pin").toInt(&ok);
        if (!ok)
        {
            device_items_disconected.push_back(item);
            continue;
        }

        mode = item->param("mode").toString().trimmed().toLower();
        if (mode.isEmpty() || (mode != "in" && mode != "out"))
        {
            device_items_disconected.push_back(item);
            continue;
        }

#ifdef NO_WIRINGPI
        state = item->raw_value().toBool();
#else
        state = digitalRead(pin) ? true : false;
#endif
        if (!item->is_connected() || item->raw_value().toBool() != state)
        {
            Device::Data_Item data_item{0, timestamp_msecs, state};
            device_items_values.emplace(item, std::move(data_item));
        }
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*, Device::Data_Item>", device_items_values),
                                  Q_ARG(bool, true));
    }

    if (!device_items_disconected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconected));
    }

    return true;
}

void WiringPiPlugin::stop() {}

void WiringPiPlugin::write(Device* dev, std::vector<Write_Cache_Item>& items)
{
    std::map<Device_Item*, Device::Data_Item> device_items_values;

    QVariant pin;
    QVariant mode;
    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    for (const Write_Cache_Item& item: items)
    {
        mode = item.dev_item_->param("mode");
        if (mode.isValid() && mode.toString().trimmed().toLower() == "out")
        {
            pin = item.dev_item_->param("pin");
            if (pin.isValid())
            {
#ifndef NO_WIRINGPI
                digitalWrite(pin.toUInt(), item.raw_data_.toBool() ? HIGH : LOW);
#endif
                Device::Data_Item data_item{item.user_id_, timestamp_msecs, item.raw_data_};
                device_items_values.emplace(item.dev_item_, std::move(data_item));
            }
        }
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*, Device::Data_Item>", device_items_values),
                                  Q_ARG(bool, true));
    }
}

} // namespace Modbus
} // namespace Das
