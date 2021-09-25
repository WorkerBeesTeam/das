#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QTimer>
#include <QCoreApplication>
#include <QThread>

#include "modbus_log.h"
#include "modbus_pack_builder.h"
#include "modbus_controller.h"

namespace Das {
namespace Modbus {

Controller::Controller(const Config& config)
    : QObject()
    , _config(config)
{
}

Controller::~Controller()
{
}

const Config& Controller::config() const
{
    return _config;
}

bool Controller::read(Device* dev)
{
    const QVector<Device_Item*>& dev_items = dev->items();
    if (dev_items.isEmpty())
        return false;

    int32_t address = Config::address(dev);

    auto find_exist = [this](int32_t address) -> Pack_Read_Manager* {
        for (auto& it : _queue._read)
            if (it._packs.front()._server_address == address)
                return &it;
        return nullptr;
    };

    {
        Pack_Builder<Device_Item*> pack_builder(dev_items);

        std::lock_guard lock(_mutex);
        Pack_Read_Manager* exist_mng = find_exist(address);

        // TODO: Concantinate same dev address items. But set values to each own device.
        if (exist_mng)
            return false;

        _queue._read.emplace_back(std::move(pack_builder._container));
    }

    QMetaObject::invokeMethod(this, &Controller::next, Qt::QueuedConnection);
    return true;
}

void Controller::write(std::vector<Write_Cache_Item> &items)
{
    if (items.empty())
        return;
    Pack_Builder<Write_Cache_Item> pack_builder(items);

    {
        std::lock_guard lock(_mutex);
        for (Pack<Write_Cache_Item>& pack: pack_builder._container)
            _queue._write.push_back(std::move(pack));
    }

    QMetaObject::invokeMethod(this, &Controller::next, Qt::QueuedConnection);
}

void Controller::set_line_use_timeout(int msec)
{
    if (msec > _line_use_timeout.count())
        _line_use_timeout = std::chrono::milliseconds(msec);
}

void Controller::write_multivalue(Device *dev, int server_address, int start_address, const QVariantList &data, bool is_coils, bool silent)
{
    QVector<quint16> values;
    for (const QVariant& val: data)
        values.push_back(val.value<quint16>());

    if (!values.size() || _modbus->state() != QModbusDevice::ConnectedState)
    {
        if (!silent)
            qWarning(Log) << "Device not connected";
        return;
    }

    auto register_type = is_coils ? QModbusDataUnit::Coils : QModbusDataUnit::HoldingRegisters;

    if (!silent)
        qInfo(Log).noquote().nospace()
            << "WRITE " << values << ' '
            << (register_type == QModbusDataUnit::Coils ? "Coils" : "HoldingRegisters")
            << " START " << start_address << " TO ADR " << server_address;

    QModbusDataUnit write_unit(register_type, start_address, values);
    auto reply = _modbus->sendWriteRequest(write_unit, server_address);
    if (reply)
    {
        int attempts = 5;
        while (!reply->isFinished() && attempts-- > 0)
        {
            qApp->processEvents();
            QThread::msleep(50);
        }

        if (!silent)
        {
            if (!reply->isFinished())
                qWarning(Log) << "Reply timeout";
            else if (reply->error() != QModbusDevice::NoError)
            {
                qWarning(Log).noquote()
                        << tr("Write response error: %1 Device address: %2 (%3) Function: %4 Start unit: %5 Data:")
                              .arg(reply->errorString())
                              .arg(reply->serverAddress())
                              .arg(reply->error() == QModbusDevice::ProtocolError ?
                                       tr("Mobus exception: 0x%1").arg(reply->rawResult().exceptionCode(), -1, 16) :
                                       tr("code: 0x%1").arg(reply->error(), -1, 16))
                              .arg(register_type).arg(start_address) << values;
            }
            else
                qInfo(Log) << "Write sucessfull";
        }

        reply->deleteLater();
    }
    else if (!silent)
        qCritical(Log).noquote() << tr("Write error:") << _modbus->errorString();
}

void Controller::process_error(QModbusDevice::Error e)
{
    _queue.clear();
    //qCCritical(Log).noquote() << "Occurred:" << e << errorString();
    if (e == QModbusDevice::ConnectionError)
        _modbus->disconnectDevice();
}

void Controller::check_state(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState)
        QMetaObject::invokeMethod(this, &Controller::next, Qt::QueuedConnection);
}

void Controller::next()
{
    if (_queue.is_active())
        return;

    const std::chrono::system_clock::duration elapsed_time = std::chrono::system_clock::now() - _line_use_last_time;
    if (elapsed_time < _line_use_timeout)
    {
        if (!_process_queue_timer->isActive())
        {
            auto elapsed_msec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);
            _process_queue_timer->start((_line_use_timeout - elapsed_msec).count());
        }
        return;
    }

    if (_queue._write.empty() && _queue._read.empty())
    {
        _modbus->disconnectDevice();
        return;
    }

    if (!check_connection())
        return;

    if (!_queue._write.empty())
    {
        Pack<Write_Cache_Item>& pack = _queue._write.front();
        write_pack(pack._server_address, pack._register_type, pack._start_address, pack._items, &pack._reply);
        if (!pack._reply)
        {
            _queue._write.pop_front();
            next();
        }
    }
    else if (!_queue._read.empty())
    {
        Pack_Read_Manager& pack_read_manager = _queue._read.front();
        ++pack_read_manager._position;
        if (pack_read_manager._position >= static_cast<int>(pack_read_manager._packs.size()))
        {
            _queue._read.pop_front();
            next();
        }
        else
        {
            Pack<Device_Item*>& pack = pack_read_manager._packs.at(pack_read_manager._position);
            read_pack(pack._server_address, pack._register_type, pack._start_address, pack._items, &pack._reply);
            if (!pack._reply)
                next();
        }
    }
}

void Controller::read_finished_slot()
{
    read_finished(qobject_cast<QModbusReply*>(sender()));
}

void Controller::write_finished_slot()
{
    write_finished(qobject_cast<QModbusReply*>(sender()));
}

bool Controller::check_connection()
{
    if (_modbus->state() == QModbusDevice::ConnectedState)
        return true;

    // TODO: Renew auto connection (see git log)

    if (_modbus->state() != QModbusDevice::UnconnectedState)
    {
        const std::chrono::system_clock::duration elapsed_time = std::chrono::system_clock::now() - _line_use_last_time;
        if (elapsed_time > std::chrono::seconds(15))
            _modbus->disconnectDevice(); // TODO: timeout for processing states
        return false;
    }
    else
    {
        Config::set(_config, _modbus.get());

        _line_use_last_time = std::chrono::system_clock::now();
        if (_modbus->connectDevice())
            return _modbus->state() == QModbusDevice::ConnectedState;
        else
            return false;
    }
}

QVector<quint16> Controller::cache_items_to_values(const std::vector<Write_Cache_Item>& items) const
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

void Controller::write_pack(int server_address, QModbusDataUnit::RegisterType register_type, int start_address,
                                    const std::vector<Write_Cache_Item>& items, QModbusReply** reply)
{
    if (items.empty())
        return;

    QVector<quint16> values = cache_items_to_values(items);
    qCDebug(DetailLog).noquote().nospace()
            << items.front().user_id_ << "|WRITE " << values << ' '
            << (register_type == QModbusDataUnit::Coils ? "Coils" : "HoldingRegisters")
            << " START " << start_address << " TO ADR " << server_address;

    QModbusDataUnit write_unit(register_type, start_address, values);
    *reply = _modbus->sendWriteRequest(write_unit, server_address);

    if (*reply)
    {
        if ((*reply)->isFinished())
            write_finished(*reply);
        else
            connect(*reply, &QModbusReply::finished, this, &Controller::write_finished_slot, Qt::DirectConnection);
    }
    else
        qCCritical(Log).noquote() << tr("Write error:") << _modbus->errorString();
}

void Controller::write_finished(QModbusReply* reply)
{
    _line_use_last_time = std::chrono::system_clock::now();
    if (!reply)
    {
        qCCritical(Log).noquote() << tr("Write finish error:") << _modbus->errorString();
        next();
        return;
    }

    if (_queue._write.empty())
        qCCritical(Log).noquote() << tr("Write finished but queue is empty");
    else
    {
        Pack<Write_Cache_Item>& pack = _queue._write.front();
        if (reply != pack._reply)
            qCCritical(Log).noquote() << tr("Write finished but is not queue front") << reply << pack._reply;

        pack._reply = nullptr;

        _queue._write.pop_front();

        if (reply->error() != QModbusDevice::NoError)
        {
            qCWarning(Log).noquote() << tr("Write response error: %1 Device address: %2 (%3) Function: %4 Start unit: %5 Data:")
                          .arg(reply->errorString())
                          .arg(reply->serverAddress())
                          .arg(reply->error() == QModbusDevice::ProtocolError ?
                                   tr("Mobus exception: 0x%1").arg(reply->rawResult().exceptionCode(), -1, 16) :
                                   tr("code: 0x%1").arg(reply->error(), -1, 16))
                          .arg(pack._register_type).arg(pack._start_address) << cache_items_to_values(pack._items);
            _queue.clear(reply->serverAddress());
        }
    }

    reply->deleteLater();
    next();
}

void Controller::read_pack(int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Device_Item*>& items, QModbusReply** reply)
{
    int count = 0;
    for (Device_Item* item : items)
        count += Config::count(item, start_address + count);

    QModbusDataUnit request(register_type, start_address, count);
    *reply = _modbus->sendReadRequest(request, server_address);

    if (*reply)
    {
        if ((*reply)->isFinished())
            read_finished(*reply);
        else
            connect(*reply, &QModbusReply::finished, this, &Controller::read_finished_slot, Qt::DirectConnection);
    }
    else
        qCCritical(Log).noquote() << tr("Read error:") << _modbus->errorString();
}

void Controller::read_finished(QModbusReply* reply)
{
    _line_use_last_time = std::chrono::system_clock::now();
    if (!reply)
    {
        qCCritical(Log).noquote() << tr("Read finish error:") << _modbus->errorString();
        next();
        return;
    }

    if (_queue._read.size())
    {
        Pack_Read_Manager& pack_read_manager = _queue._read.front();
        Pack<Device_Item*>& pack = pack_read_manager._packs.at(pack_read_manager._position);
        if (reply != pack._reply)
        {
            qCCritical(Log).noquote() << tr("Read finished but is not queue front") << reply << pack._reply;
        }

        pack._reply = nullptr;

        if (reply->error() != QModbusDevice::NoError)
        {
            pack_read_manager._is_connected = false;
            qCDebug(DetailLog) << pack._server_address << pack._register_type << reply->error()
                               << tr("Read response error: %5 Device address: %1 (%6) registerType: %2 Start: %3 Value count: %4")
                         .arg(pack._server_address).arg(pack._register_type).arg(pack._start_address).arg(pack._items.size())
                         .arg(reply->errorString())
                         .arg(reply->error() == QModbusDevice::ProtocolError ?
                                tr("Mobus exception: 0x%1").arg(reply->rawResult().exceptionCode(), -1, 16) :
                                  tr("code: 0x%1").arg(reply->error(), -1, 16));
            _queue.clear(pack._server_address);
        }
        else
        {
            int value_pos = 0;

            QVariant raw_data;
            const QModbusDataUnit unit = reply->result();
            for (uint i = 0; i < pack._items.size(); ++i)
            {
                Device_Item* item = pack._items.at(i);
                quint16 count = Config::count(item, pack._start_address + value_pos);
                QVariantList values;

                for (; count-- && value_pos < unit.valueCount(); ++value_pos)
                {
                    if (pack._register_type == QModbusDataUnit::Coils ||
                            pack._register_type == QModbusDataUnit::DiscreteInputs)
                        raw_data = static_cast<bool>(unit.value(value_pos));
                    else
                        raw_data = static_cast<qint32>(unit.value(value_pos));
                    values.push_back(raw_data);
                }

                if (values.size() > 1)
                    pack_read_manager._new_values.at(item).raw_data_ = QVariant::fromValue(values);
                else
                    pack_read_manager._new_values.at(item).raw_data_ = raw_data;
            }
        }
    }
    else
        qCCritical(Log).noquote() << tr("Read finished but queue is empty");

    reply->deleteLater();
    next();
}

void Controller::init()
{
    if (_config._tcp._address.isEmpty())
        _modbus = std::make_shared<QModbusRtuSerialMaster>();
    else
        _modbus = std::make_shared<QModbusTcpClient>();

    if (Log().isDebugEnabled())
    {
        auto dbg = qDebug(Log).noquote() << "Used as serial port:";
        if (_config._tcp._address.isEmpty())
            dbg << _config._rtu._name << "available:" << Rtu_Config::available_ports().join(", ");
        else
            dbg.nospace() << _config._tcp._address << ':' << _config._tcp._port;
    }

    connect(_modbus.get(), &QModbusClient::errorOccurred, this, &Controller::process_error, Qt::DirectConnection);
    connect(_modbus.get(), &QModbusClient::stateChanged, this, &Controller::check_state, Qt::DirectConnection);
    Config::set(_config, _modbus.get());

    _process_queue_timer = new QTimer(this);
    _process_queue_timer->setSingleShot(true);
    connect(_process_queue_timer, &QTimer::timeout, this, &Controller::next, Qt::DirectConnection);

    next();
}

void Controller::deinit()
{
    _process_queue_timer->stop();
    delete _process_queue_timer;
    _process_queue_timer = nullptr;

    _queue.clear_all();
    _modbus->disconnectDevice();
    _modbus = nullptr;
}

} // namespace Modbus
} // namespace Das
