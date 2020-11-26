#ifndef DAS_UART_CONFIG_H
#define DAS_UART_CONFIG_H

//#include <chrono>

#include <QSerialPort>

namespace Das {
namespace Uart {

struct Config
{
    Config& operator=(const Config&) = default;
    Config(const Config&) = default;
    Config(Config&&) = default;
    Config(const QString& port_name = QString(),
        int speed = QSerialPort::Baud115200,
        QSerialPort::DataBits bits_num = QSerialPort::Data8,
        QSerialPort::Parity parity = QSerialPort::NoParity,
        QSerialPort::StopBits stop_bits = QSerialPort::OneStop,
        QSerialPort::FlowControl flow_control = QSerialPort::NoFlowControl,
        int read_timeout = 3000, int write_timeout = 3000,
        const std::string& lua_script_file = {}, bool lua_use_libs = false);

    static Config& instance();

    /*static QString firstPort() {
        return QSerialPortInfo::availablePorts().count() ? QSerialPortInfo::availablePorts().first().portName() : QString();
    }*/

    static QStringList available_ports();

    static QString get_usb_serial();

    QString _name;
    int _baud_rate;   ///< Скорость. По умолчанию 115200
    QSerialPort::DataBits    _data_bits;      ///< Количество бит. По умолчанию 8
    QSerialPort::Parity      _parity;         ///< Паритет. По умолчанию нет
    QSerialPort::StopBits    _stop_bits;      ///< Стоп бит. По умолчанию 1
    QSerialPort::FlowControl _flow_control;   ///< Управление потоком. По умолчанию нет

    int _read_timeout;
    int _write_timeout;

    std::string _lua_script_file;
    bool _lua_use_libs;
};

} // namespace Uart
} // namespace Das

#endif // CONFIG_H
