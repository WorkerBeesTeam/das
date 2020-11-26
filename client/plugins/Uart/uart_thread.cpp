
#include "uart_thread.h"

namespace Das {

Q_LOGGING_CATEGORY(UartLog, "uart")

void Uart_Thread::start()
{
    Uart::Config& conf = Uart::Config::instance();

    is_port_name_in_config_ = !conf._name.isEmpty();
    if (!is_port_name_in_config_)
    {
        qCDebug(UartLog) << "No port name in config file";
        conf._name = Uart::Config::get_usb_serial();
    }

    qCDebug(UartLog).noquote() << "Used as serial port:" << conf._name << "available:" << conf.available_ports().join(", ");

    ok_open_ = true;
    break_ = false;

    QThread::start();
//    std::thread th(&Uart_Plugin::run, this);
//    thread_.swap(th);
}

bool Uart_Thread::check(Device *dev)
{
    if (!dev)
        return false;

    std::lock_guard lock(mutex_);
    for (Device_Item* item: dev->items())
    {
        if (read_set_.find(item) == read_set_.end())
        {
            read_set_.insert(item);
            read_queue_.push(item);
        }
    }
    cond_.notify_one();

    return true;
}

void Uart_Thread::stop()
{
    std::lock_guard lock(mutex_);
    break_ = true;
    cond_.notify_one();
}

void Uart_Thread::write(std::vector<Write_Cache_Item> &items)
{
    std::lock_guard lock(mutex_);
    for (Write_Cache_Item& item: items)
    {
        if (write_set_.find(item.dev_item_) == write_set_.end())
        {
            read_set_.insert(item.dev_item_);
            write_queue_.push(std::move(item));
        }
    }
    cond_.notify_one();
}

void Uart_Thread::run()
{
    try
    {
        _lua.init();
    }
    catch (const std::exception& e)
    {
        qCWarning(UartLog) << e.what();
    }
    catch(...) {}

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

    _lua.stop();
}

void Uart_Thread::set_config(QSerialPort &port)
{
    const Uart::Config& conf = Uart::Config::instance();
    port.setPortName(conf._name);
    port.setBaudRate(conf._baud_rate);
    port.setDataBits(conf._data_bits);
    port.setParity(conf._parity);
    port.setStopBits(conf._stop_bits);
    port.setFlowControl(conf._flow_control);
}

void Uart_Thread::reconnect(QSerialPort &port)
{
    if (port.isOpen())
        port.close();

    if (!is_port_name_in_config_)
        Uart::Config::instance()._name = Uart::Config::get_usb_serial();

    set_config(port);
    if (!port.open(QIODevice::ReadWrite))
    {
        if (ok_open_)
        {
            qCCritical(UartLog) << port.errorString();
            ok_open_ = false;
        }
        return;
    }

    ok_open_ = true;
    set_config(port);
}

template<typename T>
void parse_value(QDataStream& ds, Device::Data_Item& data)
{
    T value;
    ds >> value;
    data.raw_data_ = value;
}

void Uart_Thread::read_item(QSerialPort &port, Device_Item *item)
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

    if (port.waitForBytesWritten(Uart::Config::instance()._write_timeout)
        && port.waitForReadyRead(Uart::Config::instance()._read_timeout))
    {
        data = port.readAll();
    }

    if (port.error() != QSerialPort::NoError)
    {
        port.close();
        device_items_disconected_[item->device()].push_back(item);
        return;
    }

    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();
    Device::Data_Item data_item{0, timestamp_msecs, {}};

    if (_lua)
    {
        try
        {
            data_item.raw_data_ = _lua(data);
        }
        catch(const std::exception& e)
        {
            qCWarning(UartLog) << e.what();
        }
        catch(...) {}
    }
    else
    {
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
        else
            data_item.raw_data_ = data;
    }

    device_items_values_[item->device()].emplace(item, std::move(data_item));
}

void Uart_Thread::write_item(QSerialPort &port, const Write_Cache_Item &item)
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
