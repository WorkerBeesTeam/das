#include <QDebug>
#include <QSettings>
#include <QFile>

#include <Helpz/settingshelper.h>
#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/db/device_item_type.h>

#include "plugin.h"

namespace Das {

Q_LOGGING_CATEGORY(UartLog, "uart")

Uart_Plugin::Uart_Plugin() : QObject() {}

void Uart_Plugin::configure(QSettings *settings, Scheme */*scheme*/)
{
    using Helpz::Param;

    config_ = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Param<QString>,Param<QSerialPort::BaudRate>,Param<QSerialPort::DataBits>,
                            Param<QSerialPort::Parity>,Param<QSerialPort::StopBits>,
                            Param<QSerialPort::FlowControl>>
        #endif
            (
                settings, "Uart",
                Param<QString>{"Port", QString{}},
                Param<QSerialPort::BaudRate>{"BaudRate", QSerialPort::Baud9600},
                Param<QSerialPort::DataBits>{"DataBits", QSerialPort::Data8},
                Param<QSerialPort::Parity>{"Parity", QSerialPort::NoParity},
                Param<QSerialPort::StopBits>{"StopBits", QSerialPort::OneStop},
                Param<QSerialPort::FlowControl>{"FlowControl", QSerialPort::NoFlowControl}
    ).obj<Uart::Config>();

    is_port_name_in_config_ = !config_.name_.isEmpty();
    if (!is_port_name_in_config_)
    {
        qCDebug(UartLog) << "No port name in config file";
        config_.name_ = Uart::Config::get_usb_serial();
    }

    qCDebug(UartLog).noquote() << "Used as serial port:" << config_.name_ << "available:" << config_.available_ports().join(", ");

    connect(&port_, &QSerialPort::errorOccurred, [this](QSerialPort::SerialPortError e)
    {
        //qCCritical(UartLog).noquote() << "Occurred:" << e << port_.errorString();
//        if (e == ConnectionError)
//            disconnectDevice();
    });
}

bool Uart_Plugin::check(Device* dev)
{
    if (!dev)
        return false;

    std::map<Device_Item*, Device::Data_Item> device_items_values;
    std::vector<Device_Item*> device_items_disconected;

    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();
    QByteArray data;

    for (Device_Item* item: dev->items())
    {
        data = item->param("data").toByteArray();
        if (data.startsWith("0x"))
            data = QByteArray::fromHex(data.remove(0, 2));

        if (data.isEmpty())
        {
            device_items_disconected.push_back(item);
            continue;
        }

        port_.readAll();
        port_.write(data);
        port_.waitForBytesWritten();
        port_.waitForReadyRead();
        data = port_.readAll();

        Device::Data_Item data_item{0, timestamp_msecs, data};
        device_items_values.emplace(item, std::move(data_item));
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>("std::map<Device_Item*, Device::Data_Item>", device_items_values), Q_ARG(bool, true));
    }

    if (!device_items_disconected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconected));
    }

    return true;
}

void Uart_Plugin::stop() {}

void Uart_Plugin::write(std::vector<Write_Cache_Item>& items)
{
    std::map<Device_Item*, Device::Data_Item> device_items_values;
    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    for (const Write_Cache_Item& item: items)
    {
        Device::Data_Item data_item{item.user_id_, timestamp_msecs, item.raw_data_};
        device_items_values.emplace(item.dev_item_, std::move(data_item));
    }

    if (!device_items_values.empty())
    {
        Device* dev = device_items_values.begin()->first->device();
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*, Device::Data_Item>", device_items_values), Q_ARG(bool, true));
    }
}

} // namespace Das
