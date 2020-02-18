#include <QDebug>
#include <QSettings>
#include <QFile>

#include <Helpz/settingshelper.h>
#include <Das/device_item.h>
#include <Das/db/device_item_type.h>

#include "plugin.h"

namespace Das {

Q_LOGGING_CATEGORY(UartLog, "uart")

Uart_Plugin::Uart_Plugin() :
    QObject()
{}

Uart_Plugin::~Uart_Plugin()
{
    if (thread_.joinable())
    {
        stop();
        thread_.join();
    }
}

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

    break_ = false;

    std::thread th(&Uart_Plugin::run, this);
    thread_.swap(th);
}

bool Uart_Plugin::check(Device* dev)
{
    if (!dev)
        return false;

    std::lock_guard lock(mutex_);
    for (Device_Item* item: dev->items())
    {
        if (read_set_.find(item) == read_set_.end())
        {
            read_queue_.push(item);
        }
    }
    cond_.notify_one();

    return true;
}

void Uart_Plugin::stop()
{
    std::lock_guard lock(mutex_);
    break_ = true;
    cond_.notify_one();
}

void Uart_Plugin::write(std::vector<Write_Cache_Item>& items)
{
    std::lock_guard lock(mutex_);
    for (Write_Cache_Item& item: items)
    {
        if (write_set_.find(item.dev_item_) == write_set_.end())
        {
            write_queue_.push(std::move(item));
        }
    }
    cond_.notify_one();
}

void Uart_Plugin::run()
{
    QSerialPort port;

    std::unique_lock lock(mutex_, std::defer_lock);
    while(!break_)
    {
        lock.lock();
        cond_.wait(lock, [this]()
        {
            return break_
                    || !read_queue_.empty()
                    || !write_queue_.empty()
                    || !device_items_values_.empty()
                    || !device_items_disconected_.empty();
        });
        if (break_)
            break;

        if (!read_queue_.empty())
        {
            Device_Item* item = read_queue_.front();
            read_queue_.pop();
            lock.unlock();

            read_item(port, item);
        }
        else if (!write_queue_.empty())
        {
            Write_Cache_Item item = std::move(write_queue_.front());
            write_queue_.pop();
            lock.unlock();

            write_item(port, item);
        }
        else if (!device_items_values_.empty() || !device_items_disconected_.empty())
        {
            read_set_.clear();
            write_set_.clear();
            lock.unlock();

            for (const std::pair<Device*, std::map<Device_Item*, Device::Data_Item>>& it: device_items_values_)
            {
                if (!it.second.empty())
                {
                    QMetaObject::invokeMethod(it.first, "set_device_items_values", Qt::QueuedConnection,
                                              QArgument<std::map<Device_Item*, Device::Data_Item>>
                                              ("std::map<Device_Item*, Device::Data_Item>", it.second),
                                              Q_ARG(bool, true));
                }
            }
            device_items_values_.clear();

            for (const std::pair<Device*, std::vector<Device_Item*>>& it: device_items_disconected_)
            {
                if (!it.second.empty())
                {
                    QMetaObject::invokeMethod(it.first, "set_device_items_disconnect", Qt::QueuedConnection,
                                              Q_ARG(std::vector<Device_Item*>, it.second));
                }
            }
            device_items_disconected_.clear();
        }
        else
            lock.unlock();
    }
}

void Uart_Plugin::set_config(QSerialPort &port)
{
    port.setPortName(config_.name_);
    port.setBaudRate(config_.baud_rate_);
    port.setDataBits(config_.data_bits_);
    port.setParity(config_.parity_);
    port.setStopBits(config_.stop_bits_);
    port.setFlowControl(config_.flow_control_);
}

void Uart_Plugin::reconnect(QSerialPort &port)
{
    if (port.isOpen())
        port.close();

    if (!is_port_name_in_config_)
        config_.name_ = Uart::Config::get_usb_serial();

    set_config(port);
    if (!port.open(QIODevice::ReadWrite))
    {
        qCCritical(UartLog) << port.errorString();
        return;
    }

    set_config(port);
}

template<typename T>
void parse_value(QDataStream& ds, Device::Data_Item& data)
{
    T value;
    ds >> value;
    data.raw_data_ = value;
}

void Uart_Plugin::read_item(QSerialPort &port, Device_Item *item)
{
    if (!port.isOpen())
    {
        reconnect(port);

        if (!port.isOpen())
        {
            device_items_disconected_[item->device()].push_back(item);
            return;
        }
    }

    QByteArray data = item->param("data").toByteArray();
    if (data.startsWith("0x"))
        data = QByteArray::fromHex(data.remove(0, 2));

    if (data.isEmpty())
    {
        device_items_disconected_[item->device()].push_back(item);
        return;
    }

    port.readAll();
    port.write(data);
    port.waitForBytesWritten();
    port.waitForReadyRead();
    data = port.readAll();

    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();
    Device::Data_Item data_item{0, timestamp_msecs, data};

    QVariant is_number = item->device()->param("is_number");
    if (is_number.isValid() && is_number.toBool())
    {
        QVariant is_little_endian = item->device()->param("is_little_endian");
        QDataStream::ByteOrder bo = is_little_endian.isValid() && is_little_endian.toBool() ?
                    QDataStream::LittleEndian : QDataStream::BigEndian;

        QDataStream ds(data);
        ds.setByteOrder(bo);

        switch (data.size()) {
        case 1: parse_value<uint8_t>(ds, data_item); break;
        case 2: parse_value<uint16_t>(ds, data_item); break;
        case 4: parse_value<uint32_t>(ds, data_item); break;
        case 8: parse_value<qint64>(ds, data_item); break;

        default:
            data_item.raw_data_ = QVariant();
            break;
        }
    }

    device_items_values_[item->device()].emplace(item, std::move(data_item));
}

void Uart_Plugin::write_item(QSerialPort &port, const Write_Cache_Item &item)
{
    if (!port.isOpen())
    {
        device_items_disconected_[item.dev_item_->device()].push_back(item.dev_item_);
        return;
    }

    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();
    Device::Data_Item data_item{item.user_id_, timestamp_msecs, item.raw_data_};
    device_items_values_[item.dev_item_->device()].emplace(item.dev_item_, std::move(data_item));

    // TODO: write data to port
}

} // namespace Das
