#include <QSerialPortInfo>
#include <QVariant>

#include "config.h"

namespace Das {
namespace Uart {

Config::Config(const QString& port_name, QSerialPort::BaudRate speed, QSerialPort::DataBits bits_num, QSerialPort::Parity parity,
               QSerialPort::StopBits stop_bits, QSerialPort::FlowControl flow_control) :
    name_(port_name),
    baud_rate_(speed),
    data_bits_(bits_num),
    parity_(parity),
    stop_bits_(stop_bits),
    flow_control_(flow_control)
{
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
