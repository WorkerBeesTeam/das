#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QSerialPortInfo>
#include <QVariant>
#include <QUrl>
#include <QUrlQuery>

#include "config.h"

namespace Das {
namespace Modbus {

/*static*/ QString Rtu_Config::getUSBSerial()
{
    for (auto& it: QSerialPortInfo::availablePorts())
        if (it.portName().indexOf("ttyUSB") != -1)
            return it.portName();
    return QString();
}

template<typename T>
T get_query_value(const QUrlQuery& q, const QString& key, T def_value)
{
    if (q.hasQueryItem(key))
    {
        const QString raw = q.queryItemValue(key);
        if (!raw.isEmpty())
        {
            bool ok;
            int value = raw.toInt(&ok);
            if (ok)
                return static_cast<T>(value);
        }
    }
    return def_value;
}

Rtu_Config::Rtu_Config(const QString &port_name, QSerialPort::BaudRate speed, QSerialPort::DataBits bits_num,
                       QSerialPort::Parity parity, QSerialPort::StopBits stop_bits, QSerialPort::FlowControl flow_control,
                       int frame_delay_mcs) :
    _name{port_name}, _baud_rate{speed}, _data_bits{bits_num}, _parity{parity}, _stop_bits{stop_bits},
    _flow_control{flow_control}, _frame_delay_microseconds{frame_delay_mcs}
{
}

Rtu_Config::Rtu_Config(const QString &address, int frame_delay_mcs) : // rtu:///dev/ttyUSB0?baudRate=4200&stopBit=1
    _frame_delay_microseconds(frame_delay_mcs)
{
    QUrl url{address};
    if (!url.isValid() || url.scheme().toLower() != "rtu")
        return;

    QUrlQuery q(url);

    _name = url.path();
    _baud_rate = get_query_value(q, "baudRate", 115200);
    _data_bits = get_query_value(q, "dataBits", QSerialPort::Data8);
    _parity = get_query_value(q, "parity", QSerialPort::NoParity);
    _stop_bits = get_query_value(q, "stopBits", QSerialPort::OneStop);
    _flow_control = get_query_value(q, "flowControl", QSerialPort::NoFlowControl);
}

/*static*/ QStringList Rtu_Config::available_ports()
{
    QStringList ports;
    for (auto&& port: QSerialPortInfo::availablePorts())
        ports << port.portName();
    return ports;
}

/*static*/ void Rtu_Config::set(const Rtu_Config &config, QModbusRtuSerialMaster *device)
{
    device->setConnectionParameter(QModbusDevice::SerialPortNameParameter, config._name);
    device->setConnectionParameter(QModbusDevice::SerialParityParameter,   config._parity);
    device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, config._baud_rate);
    device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, config._data_bits);
    device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, config._stop_bits);

    if (config._frame_delay_microseconds > 0)
        device->setInterFrameDelay(config._frame_delay_microseconds);
}

// --------------------

Tcp_Config::Tcp_Config(const QString &address)
{
    QUrl url{address};
    if (!url.isValid() || url.scheme().toLower() != "mtcp")
        return;

    _address = url.host();
    _port = url.port();

    if (_address == "example.com")
        _address.clear();
}

/*static*/ void Tcp_Config::set(const Tcp_Config &config, QModbusTcpClient *device)
{
    device->setConnectionParameter(QModbusDevice::NetworkAddressParameter, config._address);
    device->setConnectionParameter(QModbusDevice::NetworkPortParameter,    config._port);
}

// --------------------

Config::Config(const QString &address, const QString& port_name, QSerialPort::BaudRate speed, QSerialPort::DataBits bits_num, QSerialPort::Parity parity,
               QSerialPort::StopBits stop_bits, QSerialPort::FlowControl flow_control, int modbus_timeout, int modbus_number_of_retries,
               int frame_delay_microseconds) :
    _tcp{address},
    _rtu{port_name, speed, bits_num, parity, stop_bits, flow_control, frame_delay_microseconds},

    _modbus_timeout(modbus_timeout),
    _modbus_number_of_retries(modbus_number_of_retries)
{
}

Config::Config(const QString &address, int modbus_timeout, int modbus_number_of_retries,
               int frame_delay_microseconds) :
    _tcp{address}, _rtu{address, frame_delay_microseconds},
    _modbus_timeout(modbus_timeout),
    _modbus_number_of_retries(modbus_number_of_retries)
{
}

/*static*/ void Config::set(const Config& config, QModbusClient *device)
{
    auto rtu = qobject_cast<QModbusRtuSerialMaster*>(device);
    if (rtu)
        Rtu_Config::set(config._rtu, rtu);
    else
    {
        auto tcp = qobject_cast<QModbusTcpClient*>(device);
        if (tcp)
            Tcp_Config::set(config._tcp, tcp);
        else
            throw std::runtime_error("Unknown modbus device");
    }

    device->setTimeout(config._modbus_timeout);
    device->setNumberOfRetries(config._modbus_number_of_retries);
}

/*static*/ int32_t Config::line_use_timeout(Device *dev, bool *ok)
{
    QVariant v = dev->param("line_use_timeout");
    return v.isValid() ? v.toInt(ok) : -2;
}

/*static*/ int32_t Config::address(Device *dev, bool* ok)
{
    QVariant v = dev->param("address");
    return v.isValid() ? v.toInt(ok) : -2;
}

/*static*/ int32_t Config::unit(Device_Item *item, bool* ok)
{
    QVariant v = item->param("unit");
    return v.isValid() ? v.toInt(ok) : -2;
}

/*static*/ quint16 Config::count(Device_Item *item, int32_t unit)
{
    QVariant v = item->param("count");
    if (v.isValid())
    {
        bool ok;
        int32_t val = v.toInt(&ok);
        if (ok && val > 0 && (val + unit) <= 255)
            return val;
    }
    return 1;
}

} // namespace Modbus
} // namespace Das
