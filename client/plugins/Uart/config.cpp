#include <QSerialPortInfo>
#include <QVariant>

#include "config.h"

namespace Das {
namespace Uart {

Config::Config(const QString& port_name, int speed, QSerialPort::DataBits bits_num, QSerialPort::Parity parity,
               QSerialPort::StopBits stop_bits, QSerialPort::FlowControl flow_control, int read_timeout, int write_timeout,
               const std::string &lua_script_file, bool lua_use_libs) :
    _name(port_name),
    _baud_rate(speed),
    _data_bits(bits_num),
    _parity(parity),
    _stop_bits(stop_bits),
    _flow_control(flow_control),
    _read_timeout(read_timeout), _write_timeout(write_timeout),
    _lua_script_file(lua_script_file),
    _lua_use_libs(lua_use_libs)
{
}

Config &Config::instance()
{
    static Config config;
    return config;
}

QStringList Config::available_ports()
{
    QStringList ports;
    for (auto&& port: QSerialPortInfo::availablePorts())
        ports << port.portName();
    return ports;
}

/*static*/ QString Config::get_usb_serial()
{
    for (auto& it: QSerialPortInfo::availablePorts())
        if (it.portName().indexOf("ttyUSB") != -1)
            return it.portName();
    return QString();
}

} // namespace Uart
} // namespace Das
