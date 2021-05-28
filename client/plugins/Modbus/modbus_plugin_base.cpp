#include <deque>
#include <vector>
#include <queue>
#include <iterator>
#include <type_traits>

#include <QElapsedTimer>

#include <QDebug>
#include <QSettings>
#include <QFile>

#ifdef QT_DEBUG
#include <QtDBus>
#endif

#include <Helpz/settingshelper.h>
#include <Helpz/simplethread.h>

#include <Das/db/device_item_type.h>
#include <Das/device_item.h>
#include <Das/device.h>

#include "modbus_plugin_base.h"

namespace Das {
namespace Modbus {

QElapsedTimer tt;

Q_LOGGING_CATEGORY(ModbusLog, "modbus")
Q_LOGGING_CATEGORY(ModbusDetailLog, "modbus.detail", QtInfoMsg)

template <typename T>
struct Modbus_Pack_Item_Cast {
    static inline Device_Item* run(T item) { return item; }
};

template <>
struct Modbus_Pack_Item_Cast<Write_Cache_Item> {
    static inline Device_Item* run(const Write_Cache_Item& item) { return item.dev_item_; }
};

template<typename T>
struct Modbus_Pack
{
    Modbus_Pack(Modbus_Pack<T>&& o) = default;
    Modbus_Pack(const Modbus_Pack<T>& o) = default;
    Modbus_Pack<T>& operator =(Modbus_Pack<T>&& o) = default;
    Modbus_Pack<T>& operator =(const Modbus_Pack<T>& o) = default;
    Modbus_Pack(T&& item) :
        reply_(nullptr)
    {
        init(Modbus_Pack_Item_Cast<T>::run(item), std::is_same<Write_Cache_Item, T>::value);
        items_.push_back(std::move(item));
    }

    void init(Device_Item* dev_item, bool is_write)
    {
        bool ok;
        server_address_ = Modbus_Plugin_Base::address(dev_item->device(), &ok);
        if (ok && server_address_ > 0)
        {
            start_address_ = Modbus_Plugin_Base::unit(dev_item, &ok);
            if (ok && start_address_ >= 0)
            {
                if (dev_item->register_type() > QModbusDataUnit::Invalid && dev_item->register_type() <= QModbusDataUnit::HoldingRegisters &&
                        (!is_write || (dev_item->register_type() == QModbusDataUnit::Coils ||
                                       dev_item->register_type() == QModbusDataUnit::HoldingRegisters)))
                {
                    register_type_ = static_cast<QModbusDataUnit::RegisterType>(dev_item->register_type());
                }
                else
                    register_type_ = QModbusDataUnit::Invalid;
            }
            else
                start_address_ = -1;
        }
        else
            server_address_ = -1;
    }

    bool is_valid() const
    {
        return server_address_ > 0 && start_address_ >= 0 && register_type_ != QModbusDataUnit::Invalid;
    }

    bool add_next(T&& item)
    {
        Device_Item* dev_item = Modbus_Pack_Item_Cast<T>::run(item);
        if (register_type_ == dev_item->register_type() &&
            server_address_ == Modbus_Plugin_Base::address(dev_item->device()))
        {
            int unit = Modbus_Plugin_Base::unit(dev_item);
            if (unit == (start_address_ + static_cast<int>(items_.size())))
            {
                items_.push_back(std::move(item));
                return true;
            }
        }
        return false;
    }

    bool assign(Modbus_Pack<T>& pack)
    {
        if (register_type_ == pack.register_type_ &&
            server_address_ == pack.server_address_ &&
            (start_address_ + static_cast<int>(items_.size())) == pack.start_address_)
        {
            std::copy( std::make_move_iterator(pack.items_.begin()),
                       std::make_move_iterator(pack.items_.end()),
                       std::back_inserter(items_) );
            return true;
        }
        return false;
    }

    bool operator <(Device_Item* dev_item) const
    {
        return register_type_ < dev_item->register_type() ||
               server_address_ < Modbus_Plugin_Base::address(dev_item->device()) ||
               (start_address_ + static_cast<int>(items_.size())) < Modbus_Plugin_Base::unit(dev_item);
    }

    int server_address_;
    int start_address_;
    QModbusDataUnit::RegisterType register_type_;
    QModbusReply* reply_;

    std::vector<T> items_;
};

template<typename T, typename Container> struct Input_Container_Device_Item_Type { typedef T& type; };
template<typename T, typename Container> struct Input_Container_Device_Item_Type<T, const Container> { typedef T type; };

template<typename T>
class Modbus_Pack_Builder
{
public:
    template<typename Input_Container>
    Modbus_Pack_Builder(Input_Container& items)
    {
        typename std::vector<Modbus_Pack<T>>::iterator it;

        for (typename Input_Container_Device_Item_Type<T, Input_Container>::type item: items)
        {
            insert(std::move(item));
        }
    }

    std::vector<Modbus_Pack<T>> container_;
private:
    void insert(T&& item)
    {
        typename std::vector<Modbus_Pack<T>>::iterator it = container_.begin();
        for (; it != container_.end(); ++it)
        {
            if (*it < Modbus_Pack_Item_Cast<T>::run(item))
            {
                continue;
            }
            else if (it->add_next(std::move(item)))
            {
                assign_next(it);
                return;
            }
            else
            {
                create(it, std::move(item));
                return;
            }
        }

        if (it == container_.end())
        {
            create(it, std::move(item));
        }
    }

    void create(typename std::vector<Modbus_Pack<T>>::iterator it, T&& item)
    {
        Modbus_Pack pack(std::move(item));
        if (pack.is_valid())
        {
            it = container_.insert(it, std::move(pack));
            assign_next(it);
        }
    }

    void assign_next(typename std::vector<Modbus_Pack<T>>::iterator it)
    {
        if (it != container_.end())
        {
            typename std::vector<Modbus_Pack<T>>::iterator old_it = it;
            it++;
            if (it != container_.end())
            {
                if (old_it->assign(*it))
                {
                    container_.erase(it);
                }
            }
        }
    }
};

// -----------------------------------------------------------------

class Modbus_Pack_Read_Manager
{
public:
    Modbus_Pack_Read_Manager(const Modbus_Pack_Read_Manager&) = delete;
    Modbus_Pack_Read_Manager& operator =(const Modbus_Pack_Read_Manager&) = delete;
    Modbus_Pack_Read_Manager(std::vector<Modbus_Pack<Device_Item*>>&& packs) :
        is_connected_(true), position_(-1), packs_(std::move(packs))
    {
        qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

        for (Modbus_Pack<Device_Item*> &item_pack : packs_)
        {
            for (Device_Item* device_item : item_pack.items_)
            {
                new_values_.emplace(device_item, Device::Data_Item{0, timestamp_msecs, {}});
            }
        }
    }

    Modbus_Pack_Read_Manager(Modbus_Pack_Read_Manager&& o) :
        is_connected_(std::move(o.is_connected_)), position_(std::move(o.position_)), packs_(std::move(o.packs_)), new_values_(std::move(o.new_values_))
    {
        o.packs_.clear();
        o.new_values_.clear();
    }

    ~Modbus_Pack_Read_Manager()
    {
        if (!is_connected_)
        {
            std::vector<Device_Item*> v;
            for (auto it : packs_)
            {
              v.insert(v.end(), it.items_.begin(), it.items_.end());
            }
            QMetaObject::invokeMethod(packs_.front().items_.front()->device(), "set_device_items_disconnect", Q_ARG(std::vector<Device_Item*>, v));
        }
        else if (packs_.size())
        {
            QMetaObject::invokeMethod(packs_.front().items_.front()->device(), "set_device_items_values",
                                      QArgument<std::map<Device_Item*, Device::Data_Item>>("std::map<Device_Item*,Device::Data_Item>", new_values_), Q_ARG(bool, true));
        }
        while (packs_.size())
        {
            if (packs_.front().reply_)
                QObject::disconnect(packs_.front().reply_, 0, 0, 0);
            packs_.pop_back();
        }
    }

    bool is_connected_;
    int position_; // int becose -1 is default
    std::vector<Modbus_Pack<Device_Item*>> packs_;
    std::map<Device_Item*, Device::Data_Item> new_values_;
};


// -----------------------------------------------------------------

struct Modbus_Queue
{
    std::deque<Modbus_Pack<Write_Cache_Item>> write_;
    std::deque<Modbus_Pack_Read_Manager> read_;

    bool is_active() const
    {
        if (!read_.empty())
        {
            int position  = read_.front().position_;
            if (position > -1 && read_.front().packs_.at(position).reply_)
            {
                return true;
            }
        }

        return write_.size() && write_.front().reply_;
    }

    void clear()
    {
        while (write_.size())
        {
            if (write_.front().reply_)
                QObject::disconnect(write_.front().reply_, 0, 0, 0);
            write_.pop_front();
        }

        while (read_.size())
        {
            read_.pop_front();
        }
    }

    void clear_by_address(int address)
    {
        for (std::size_t i = 0; i < write_.size(); ++i)
        {
            if (write_.front().server_address_ == address)
            {
                if (write_.front().reply_)
                    QObject::disconnect(write_.front().reply_, 0, 0, 0);
                write_.erase(write_.begin() + i);
                --i;
            }
        }

        std::deque<Modbus_Pack_Read_Manager> read;
        for (auto& it : read_)
        {
            if (it.packs_.front().server_address_ != address)
            {
                read.push_back(std::move(it));
            }
        }
        read_ = std::move(read);
    }
};

// -----------------------------------------------------------------

Modbus_Plugin_Base::Modbus_Plugin_Base() :
    base_config_(QString{}, QString{}),
    b_break(false),
    line_use_last_time_(std::chrono::system_clock::now())
{
    process_queue_timer_.setSingleShot(true);
    connect(&process_queue_timer_, &QTimer::timeout, this, &Modbus_Plugin_Base::process_queue);

    queue_ = new Modbus_Queue;
}

Modbus_Plugin_Base::~Modbus_Plugin_Base()
{
    stop();

    queue_->clear();
    delete queue_;
    queue_ = nullptr;
}

bool Modbus_Plugin_Base::check_break_flag() const
{
    return b_break;
}

void Modbus_Plugin_Base::clear_break_flag()
{
    if (b_break)
        b_break = false;
}

const Config& Modbus_Plugin_Base::base_config() const
{
    return base_config_;
}

bool Modbus_Plugin_Base::checkConnect(Device* dev)
{
    QModbusClient* modbus = modbus_by_dev(dev);
    if (modbus->state() == QModbusDevice::UnconnectedState)
    {
        Config config = dev_config(dev);
        Config::set(config, modbus);
        if (!modbus->connectDevice() && modbus->error() != QModbusDevice::NoError)
        {
            print_cached(modbus, 0, QModbusDataUnit::Invalid, QModbusDevice::ConnectionError, tr("Connect failed: ") + modbus->errorString());
            return false;
        }
    }

    return true;
}

void Modbus_Plugin_Base::configure(QSettings *settings)
{
    using Helpz::Param;

    QModbusRtuSerialMaster rtu_modbus;
    base_config_ = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Param<QString>,Param<QSerialPort::BaudRate>,Param<QSerialPort::DataBits>,
                            Param<QSerialPort::Parity>,Param<QSerialPort::StopBits>,Param<QSerialPort::FlowControl>,Param<int>,Param<int>,Param<int>>
        #endif
            (
                settings, "Modbus",
                Param<QString>{"Address", "mtcp://example.com:502"},
                Param<QString>{"Port", "ttyUSB0"},
                Param<QSerialPort::BaudRate>{"BaudRate", QSerialPort::Baud9600},
                Param<QSerialPort::DataBits>{"DataBits", QSerialPort::Data8},
                Param<QSerialPort::Parity>{"Parity", QSerialPort::NoParity},
                Param<QSerialPort::StopBits>{"StopBits", QSerialPort::OneStop},
                Param<QSerialPort::FlowControl>{"FlowControl", QSerialPort::NoFlowControl},
                Param<int>{"ModbusTimeout", rtu_modbus.timeout()},
                Param<int>{"ModbusNumberOfRetries", rtu_modbus.numberOfRetries()},
                Param<int>{"InterFrameDelay", rtu_modbus.interFrameDelay()}
    ).obj<Config>();

    auto [auto_port, line_use_timeout] = Helpz::SettingsHelper(settings, "Modbus",
            Param<int>{"AutoTTY", NoAuto},
            Param<int>{"LineUseTimeout", _line_use_timeout.count()}
    )();
    _auto_port = auto_port;
    _line_use_timeout = std::chrono::milliseconds{line_use_timeout};
}

bool Modbus_Plugin_Base::check(Device* dev)
{
    if (!checkConnect(dev))
        return false;

    clear_break_flag();

    read(dev, dev->items());
    return true;
}

void Modbus_Plugin_Base::stop()
{
    b_break = true;
}

void Modbus_Plugin_Base::write(Device *dev, std::vector<Write_Cache_Item>& items)
{
    if (!items.size() || b_break || !checkConnect(dev))
        return;

    Modbus_Pack_Builder<Write_Cache_Item> pack_builder(items);
    for (Modbus_Pack<Write_Cache_Item>& pack: pack_builder.container_)
    {
        queue_->write_.push_back(std::move(pack));
    }
    process_queue();
}

QStringList Modbus_Plugin_Base::available_ports() const
{
    return Rtu_Config::available_ports();
}

void Modbus_Plugin_Base::clear_status_cache()
{
    dev_status_cache_.clear();
}

void Modbus_Plugin_Base::write_multivalue(Device* dev, int server_address, int start_address, const QVariantList &data,
                                          bool is_coils, bool silent)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, "write_multivalue", Qt::QueuedConnection,
                                  Q_ARG(Device*, dev), Q_ARG(int, server_address), Q_ARG(int, start_address),
                                  Q_ARG(QVariantList, data), Q_ARG(bool, is_coils), Q_ARG(bool, silent));
        return;
    }

    QModbusClient* modbus = modbus_by_dev(dev);

    QVector<quint16> values;
    for (const QVariant& val: data)
        values.push_back(val.value<quint16>());

    if (!values.size() || modbus->state() != QModbusDevice::ConnectedState)
    {
        if (!silent)
            qWarning(ModbusLog) << "Device not connected";
        return;
    }

    auto register_type = is_coils ? QModbusDataUnit::Coils : QModbusDataUnit::HoldingRegisters;

    if (!silent)
        qInfo(ModbusLog).noquote().nospace()
            << "WRITE " << values << ' '
            << (register_type == QModbusDataUnit::Coils ? "Coils" : "HoldingRegisters")
            << " START " << start_address << " TO ADR " << server_address;

    QModbusDataUnit write_unit(register_type, start_address, values);
    auto reply = modbus->sendWriteRequest(write_unit, server_address);

    if (reply)
    {
        int attempts = 5;
        while (!reply->isFinished() && attempts-- > 0)
            QThread::msleep(50);

        if (!silent)
        {
            if (!reply->isFinished())
                qWarning(ModbusLog) << "Reply timeout";
            else if (reply->error() != QModbusDevice::NoError)
            {
                qWarning(ModbusLog).noquote()
                        << tr("Write response error: %1 Device address: %2 (%3) Function: %4 Start unit: %5 Data:")
                              .arg(reply->errorString())
                              .arg(reply->serverAddress())
                              .arg(reply->error() == QModbusDevice::ProtocolError ?
                                       tr("Mobus exception: 0x%1").arg(reply->rawResult().exceptionCode(), -1, 16) :
                                       tr("code: 0x%1").arg(reply->error(), -1, 16))
                              .arg(register_type).arg(start_address) << values;
            }
            else
                qInfo(ModbusLog) << "Write sucessfull";
        }

        reply->deleteLater();
    }
    else if (!silent)
        qCritical(ModbusLog).noquote() << tr("Write error: ") + modbus->errorString();
}

void Modbus_Plugin_Base::read_finished_slot()
{
    read_finished(_last_modbus, qobject_cast<QModbusReply*>(sender()));
}

void Modbus_Plugin_Base::write_finished_slot()
{
    write_finished(_last_modbus, qobject_cast<QModbusReply*>(sender()));
}

void Modbus_Plugin_Base::clear_queue()
{
    queue_->clear();
}

void Modbus_Plugin_Base::print_cached(QModbusClient *modbus, int server_address, QModbusDataUnit::RegisterType register_type,
                                      QModbusDevice::Error value, const QString& text)
{
    Status_Holder request_pair{server_address, register_type, reinterpret_cast<qintptr>(modbus)};
    auto status_it = dev_status_cache_.find(request_pair);

    if (status_it == dev_status_cache_.end() || status_it->second != value)
    {
        qCWarning(ModbusLog).noquote() << text;

        if (status_it == dev_status_cache_.end())
            dev_status_cache_[request_pair] = value;
        else
            status_it->second = value;
    }
}

QModbusClient* Modbus_Plugin_Base::check_connection(Device* dev)
{
    QModbusClient* modbus = modbus_by_dev(dev);

    if (modbus->state() == QModbusDevice::ConnectedState)
        return modbus;

    Config config = dev_config(dev);

    if (config._tcp._address.isEmpty() && config._rtu._name.isEmpty())
    {
        print_cached(modbus, 0, QModbusDataUnit::Invalid, QModbusDevice::ConnectionError, tr("No port available"));
        return nullptr;
    }

    Status_Holder request_pair{0, QModbusDataUnit::Invalid, reinterpret_cast<qintptr>(modbus)};
    if (modbus->state() != QModbusDevice::UnconnectedState)
    {
        auto status_it = dev_status_cache_.find(request_pair);

        if (status_it != dev_status_cache_.end())
        {
            modbus->disconnectDevice(); // TODO: timeout for processing states

            if (modbus->state() != QModbusDevice::UnconnectedState)
                return nullptr;
        }
        else
            return nullptr;
    }

    Config::set(config, modbus);

    if (modbus->connectDevice())
    {
        auto status_it = dev_status_cache_.find(request_pair);
        if (status_it != dev_status_cache_.end())
        {
            qCDebug(ModbusLog) << "Modbus device opened";
            dev_status_cache_.erase(status_it);
        }

        return modbus;
    }
    else
    {
        QString port_name;
        if (config._tcp._address.isEmpty())
            port_name = config._rtu._name;
        else
            port_name = config._tcp._address + ':' + QString::number(config._tcp._port);

        print_cached(modbus, 0, QModbusDataUnit::Invalid, modbus->error(), tr("Connect to port %1 fail: %2 %3")
                     .arg(port_name).arg(modbus->error()).arg(modbus->errorString()));
    }
    return nullptr;
}

void Modbus_Plugin_Base::read(Device *dev, const QVector<Device_Item*>& dev_items)
{
    if (dev_items.isEmpty())
        return;

    for (auto& it : queue_->read_)
        if (it.packs_.front().server_address_ == Modbus_Plugin_Base::address(dev))
            return;

//    qint64 elapsed = tt.restart();
//    qWarning().nospace() << ">>>> read " << (elapsed < 100 ? (elapsed < 10 ? "  " : " ") : "") <<  elapsed << " \tsize " << dev_items.size() << ' ' << dev_items.front()->device()->toString();

    Modbus_Pack_Builder<Device_Item*> pack_builder(dev_items);
    Modbus_Pack_Read_Manager mng(std::move(pack_builder.container_));
    queue_->read_.push_back(std::move(mng));
    process_queue();
}

void Modbus_Plugin_Base::process_queue()
{
    if (!b_break && !queue_->is_active())
    {
        const std::chrono::system_clock::duration elapsed_time = std::chrono::system_clock::now() - line_use_last_time_;
        if (elapsed_time < _line_use_timeout)
        {
            if (!process_queue_timer_.isActive())
            {
                auto elapsed_msec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);
                process_queue_timer_.start((_line_use_timeout - elapsed_msec).count());
            }
            return;
        }

        if (queue_->write_.size())
        {
            Modbus_Pack<Write_Cache_Item>& pack = queue_->write_.front();
            Device* dev = pack.items_.front().dev_item_->device();
            QModbusClient* modbus = check_connection(dev);
            if (modbus)
            {
                write_pack(modbus, pack.server_address_, pack.register_type_, pack.start_address_, pack.items_, &pack.reply_);
                if (!pack.reply_)
                {
                    queue_->write_.pop_front();
                    process_queue();
                }
            }
            else
                queue_->write_.pop_front();
        }
        else if (queue_->read_.size())
        {
            Modbus_Pack_Read_Manager& modbus_pack_read_manager = queue_->read_.front();
            ++modbus_pack_read_manager.position_;
            if (modbus_pack_read_manager.position_ >= static_cast<int>(modbus_pack_read_manager.packs_.size()))
            {
                queue_->read_.pop_front();
                process_queue();
            }
            else
            {
                Modbus_Pack<Device_Item*>& pack = modbus_pack_read_manager.packs_.at(modbus_pack_read_manager.position_);
                Device* dev = pack.items_.front()->device();

                QModbusClient* modbus = check_connection(dev);
                if (modbus)
                {
    //                qint64 elapsed = tt.restart();
    //                qWarning().nospace() << "->>>> read " << (elapsed < 100 ? (elapsed < 10 ? "  " : " ") : "") <<  elapsed << " \tsize " << pack.items_.size() << ' ' << pack.items_.front()->device()->toString();
                    read_pack(modbus, pack.server_address_, pack.register_type_, pack.start_address_, pack.items_, &pack.reply_);

                    if (!pack.reply_)
                        process_queue();
                }
                else
                    queue_->read_.pop_front();
            }
        }
    }
    else if (b_break && queue_)
        queue_->clear();
}

QVector<quint16> Modbus_Plugin_Base::cache_items_to_values(const std::vector<Write_Cache_Item>& items) const
{
    QVector<quint16> values;
    auto add_value = [&values](const QVariant& value)
    {
        quint16 write_data;
        if (value.type() == QVariant::Bool)
            write_data = value.toBool() ? 1 : 0;
        else
            write_data = value.toUInt();
        values.push_back(write_data);
    };

    for (const Write_Cache_Item& item: items)
    {
        if (item.raw_data_.type() == QVariant::List)
        {
           const QList<QVariant> value_list = item.raw_data_.toList();
           for (const QVariant& value: value_list)
               add_value(value);
        }
        else
            add_value(item.raw_data_);
    }
    return values;
}

void Modbus_Plugin_Base::write_pack(QModbusClient *modbus, int server_address, QModbusDataUnit::RegisterType register_type, int start_address,
                                    const std::vector<Write_Cache_Item>& items, QModbusReply** reply)
{
    if (!items.size() || modbus->state() != QModbusDevice::ConnectedState)
        return;

    QVector<quint16> values = cache_items_to_values(items);
    qCDebug(ModbusDetailLog).noquote().nospace()
            << items.front().user_id_ << "|WRITE " << values << ' '
            << (register_type == QModbusDataUnit::Coils ? "Coils" : "HoldingRegisters")
            << " START " << start_address << " TO ADR " << server_address;

    QModbusDataUnit write_unit(register_type, start_address, values);
    *reply = modbus->sendWriteRequest(write_unit, server_address);

    if (*reply)
    {
        if ((*reply)->isFinished())
            write_finished(modbus, *reply);
        else
        {
            _last_modbus = modbus;
            connect(*reply, &QModbusReply::finished, this, &Modbus_Plugin_Base::write_finished_slot);
        }
    }
    else
        qCCritical(ModbusLog).noquote() << tr("Write error: ") + modbus->errorString();
}

void Modbus_Plugin_Base::write_finished(QModbusClient *modbus, QModbusReply* reply)
{
    line_use_last_time_ = std::chrono::system_clock::now();
    if (!reply || b_break)
    {
        qCCritical(ModbusLog).noquote() << tr("Write finish error: ") + modbus->errorString();
        if (reply)
            reply->deleteLater();
        process_queue();
        return;
    }

    if (queue_->write_.size())
    {
        Modbus_Pack<Write_Cache_Item>& pack = queue_->write_.front();
        if (reply != pack.reply_)
        {
            qCCritical(ModbusLog).noquote() << tr("Write finished but is not queue front") << reply << pack.reply_;
        }

        pack.reply_ = nullptr;

        queue_->write_.pop_front();

        if (reply->error() != QModbusDevice::NoError)
        {
            qCWarning(ModbusLog).noquote() << tr("Write response error: %1 Device address: %2 (%3) Function: %4 Start unit: %5 Data:")
                          .arg(reply->errorString())
                          .arg(reply->serverAddress())
                          .arg(reply->error() == QModbusDevice::ProtocolError ?
                                   tr("Mobus exception: 0x%1").arg(reply->rawResult().exceptionCode(), -1, 16) :
                                   tr("code: 0x%1").arg(reply->error(), -1, 16))
                          .arg(pack.register_type_).arg(pack.start_address_) << cache_items_to_values(pack.items_);
            queue_->clear_by_address(reply->serverAddress());
        }
    }
    else
        qCCritical(ModbusLog).noquote() << tr("Write finished but queue is empty");

    reply->deleteLater();
    process_queue();
}

void Modbus_Plugin_Base::read_pack(QModbusClient *modbus, int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Device_Item*>& items, QModbusReply** reply)
{
    QModbusDataUnit request(register_type, start_address, items.size());
    *reply = modbus->sendReadRequest(request, server_address);

    if (*reply)
    {
        if ((*reply)->isFinished())
            read_finished(modbus, *reply);
        else
        {
            _last_modbus = modbus;
            connect(*reply, &QModbusReply::finished, this, &Modbus_Plugin_Base::read_finished_slot);
        }
    }
    else
        qCCritical(ModbusLog).noquote() << tr("Read error: ") + modbus->errorString();
}

void Modbus_Plugin_Base::read_finished(QModbusClient *modbus, QModbusReply* reply)
{
    line_use_last_time_ = std::chrono::system_clock::now();
    if (!reply || b_break)
    {
        qCCritical(ModbusLog).noquote() << tr("Read finish error: ") + modbus->errorString();
        if (reply)
            reply->deleteLater();
        process_queue();
        return;
    }

    if (queue_->read_.size())
    {
        Modbus_Pack_Read_Manager& modbus_pack_read_manager = queue_->read_.front();
        Modbus_Pack<Device_Item*>& pack = modbus_pack_read_manager.packs_.at(modbus_pack_read_manager.position_);
        if (reply != pack.reply_)
        {
            qCCritical(ModbusLog).noquote() << tr("Read finished but is not queue front") << reply << pack.reply_;
        }

        pack.reply_ = nullptr;

        if (reply->error() != QModbusDevice::NoError)
        {
            modbus_pack_read_manager.is_connected_ = false;
            print_cached(modbus, pack.server_address_, pack.register_type_, reply->error(), tr("Read response error: %5 Device address: %1 (%6) registerType: %2 Start: %3 Value count: %4")
                         .arg(pack.server_address_).arg(pack.register_type_).arg(pack.start_address_).arg(pack.items_.size())
                         .arg(reply->errorString())
                         .arg(reply->error() == QModbusDevice::ProtocolError ?
                                tr("Mobus exception: 0x%1").arg(reply->rawResult().exceptionCode(), -1, 16) :
                                  tr("code: 0x%1").arg(reply->error(), -1, 16)));
            queue_->clear_by_address(pack.server_address_);
        }
        else
        {
            QVariant raw_data;
            const QModbusDataUnit unit = reply->result();
            for (uint i = 0; i < pack.items_.size(); ++i)
            {
                if (i < unit.valueCount())
                {
                    if (pack.register_type_ == QModbusDataUnit::Coils ||
                            pack.register_type_ == QModbusDataUnit::DiscreteInputs)
                    {
                        raw_data = static_cast<bool>(unit.value(i));
                    }
                    else
                    {
                        raw_data = static_cast<qint32>(unit.value(i));
                    }
                }

                modbus_pack_read_manager.new_values_.at(pack.items_.at(i)).raw_data_ = raw_data;
    //                QMetaObject::invokeMethod(pack.items_.at(i), "set_raw_value", Qt::QueuedConnection, Q_ARG(const QVariant&, raw_data));
            }

            Status_Holder request_pair{pack.server_address_, pack.register_type_, reinterpret_cast<qintptr>(modbus)};
            auto status_it = dev_status_cache_.find(request_pair);
            if (status_it != dev_status_cache_.end())
            {
                qCDebug(ModbusLog) << "Modbus device" << pack.server_address_ << "recovered" << status_it->second
                         << "RegisterType:" << pack.register_type_ << "Start:" << pack.start_address_ << "Value count:" << pack.items_.size();
                dev_status_cache_.erase(status_it);
            }
        }
    }
    else
        qCCritical(ModbusLog).noquote() << tr("Read finished but queue is empty");

    reply->deleteLater();
    process_queue();
}

QModbusClient *Modbus_Plugin_Base::modbus_by_dev(Device *dev)
{
    for (auto it = _dev_map.cbegin(); it != _dev_map.cend(); ++it)
        if (std::find(it->second.cbegin(), it->second.cend(), dev) != it->second.cend())
            return it->first.get();

    Config config = dev_config(dev, /*simple=*/true);

    auto it = _dev_map.begin();
    for (; it != _dev_map.end(); ++it)
    {
        if (std::find_if(it->second.cbegin(), it->second.cend(), [this, &config](Device* d)
        {
            Config d_config = dev_config(d, true);
            if (!config._tcp._address.isEmpty())
                return config._tcp._address == d_config._tcp._address
                    && config._tcp._port == d_config._tcp._port;
            else
                return config._rtu._name == d_config._rtu._name;
        }) != it->second.cend())
        {
            break;
        }
    }
    if (it != _dev_map.end())
    {
        it->second.push_back(dev);
        return it->first.get();
    }
    else
    {
        std::shared_ptr<QModbusClient> modbus = init_modbus(dev);
        _dev_map.emplace(modbus, std::vector<Device*>{dev});
        return modbus.get();
    }
}

std::shared_ptr<QModbusClient> Modbus_Plugin_Base::init_modbus(Device *dev)
{
    Config config = dev_config(dev);

    std::shared_ptr<QModbusClient> modbus;

    if (config._tcp._address.isEmpty())
    {
        modbus = std::make_shared<QModbusRtuSerialMaster>();

#if defined(QT_DEBUG) && defined(Q_OS_UNIX)
        if (QDBusConnection::sessionBus().isConnected())
        {
            QDBusInterface iface("ru.deviceaccess.Das.Emulator", "/", "", QDBusConnection::sessionBus());
            if (iface.isValid())
            {
                QDBusReply<QString> tty_path = iface.call("ttyPath");
                if (tty_path.isValid())
                    config._rtu._name = tty_path.value(); // "/dev/ttyUSB0";
            }
            else
            {
                QString overTCP = "/home/kirill/vmodem0";
                config._rtu._name = QFile::exists(overTCP) ? overTCP : config._rtu._name;//"/dev/pts/10";
            }
        }
#endif
    }
    else
        modbus = std::make_shared<QModbusTcpClient>();

    if (ModbusLog().isDebugEnabled())
    {
        auto dbg = qDebug(ModbusLog).noquote() << "Used as serial port:";
        if (config._tcp._address.isEmpty())
            dbg << config._rtu._name << "available:" << Rtu_Config::available_ports().join(", ");
        else
            dbg.nospace() << config._tcp._address << ':' << config._tcp._port;
    }

    connect(modbus.get(), &QModbusClient::errorOccurred, [this, modbus](QModbusDevice::Error e)
    {
        queue_->clear();
        //qCCritical(ModbusLog).noquote() << "Occurred:" << e << errorString();
        if (e == QModbusDevice::ConnectionError)
            modbus->disconnectDevice();
    });

    Config::set(config, modbus.get());
    return modbus;
}

Config Modbus_Plugin_Base::dev_config(Device *dev, bool simple)
{
    const QString conn_str = dev->param("conn_str").toString();
    Config config(conn_str,
                  base_config_._modbus_timeout,
                  base_config_._modbus_number_of_retries,
                  base_config_._rtu._frame_delay_microseconds);

    if (!simple && !config._rtu._name.isEmpty())
    {
        if (config._rtu._name == "/auto_usb")
            config._rtu._name = Rtu_Config::getUSBSerial();
        else if (config._rtu._name == "/auto_tty")
        {
            auto ports = Rtu_Config::available_ports();
            if (!ports.empty())
                config._rtu._name = ports.front();
        }
    }

    return config;
}

/*static*/ int32_t Modbus_Plugin_Base::address(Device *dev, bool* ok)
{
    QVariant v = dev->param("address");
    return v.isValid() ? v.toInt(ok) : -2;
}

/*static*/ int32_t Modbus_Plugin_Base::unit(Device_Item *item, bool* ok)
{
    QVariant v = item->param("unit");
    return v.isValid() ? v.toInt(ok) : -2;
}

} // namespace Modbus
} // namespace Das
