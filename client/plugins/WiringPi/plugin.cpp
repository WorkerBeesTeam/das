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

void WiringPiPlugin::configure(QSettings *settings, Scheme *scheme)
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

    for (auto * device : scheme->devices())
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
    std::map<Device_Item*, QVariant> device_items_values;

    bool ok, state;
    int pin;
    QString mode;
    for (Device_Item * item: dev->items())
    {
        pin = item->param("pin").toInt(&ok);
        if (!ok)
            continue;

        mode = item->param("mode").toString().trimmed().toLower();
        if (mode.isEmpty() || (mode != "in" && mode != "out"))
            continue;

#ifdef NO_WIRINGPI
        state = item->raw_value().toBool();
#else
        state = digitalRead(pin) ? true : false;
#endif
        if (!item->is_connected() || item->raw_value().toBool() != state)
            device_items_values.emplace(item, state);
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, QVariant>>("std::map<Device_Item*, QVariant>", device_items_values),
                                  Q_ARG(bool, true));
    }

    return true;
}

void WiringPiPlugin::stop() {}

void WiringPiPlugin::write(std::vector<Write_Cache_Item>& items)
{
    QVariant pin;
    QVariant mode;
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
                QMetaObject::invokeMethod(item.dev_item_, "set_raw_value", Qt::QueuedConnection,
                                          Q_ARG(const QVariant&, item.raw_data_), Q_ARG(bool, false), Q_ARG(uint32_t, item.user_id_));
            }
        }
    }
}

} // namespace Modbus
} // namespace Das
