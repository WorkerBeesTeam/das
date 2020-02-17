#ifndef DAS_UART_CONFIG_H
#define DAS_UART_CONFIG_H

//#include <chrono>

#include <QSerialPort>

namespace Das {
namespace Uart {

struct Config {
    Config& operator=(const Config&) = default;
    Config(const Config&) = default;
    Config(Config&&) = default;
    Config(const QString& port_name = QString(),
         QSerialPort::BaudRate speed = QSerialPort::Baud115200,
         QSerialPort::DataBits bits_num = QSerialPort::Data8,
         QSerialPort::Parity parity = QSerialPort::NoParity,
         QSerialPort::StopBits stop_bits = QSerialPort::OneStop,
         QSerialPort::FlowControl flow_control = QSerialPort::NoFlowControl);

    /*static QString firstPort() {
        return QSerialPortInfo::availablePorts().count() ? QSerialPortInfo::availablePorts().first().portName() : QString();
    }*/

    static QStringList available_ports();

    static QString get_usb_serial();

    QString name_;
    int baud_rate_;   ///< Скорость. По умолчанию 115200
    QSerialPort::DataBits    data_bits_;      ///< Количество бит. По умолчанию 8
    QSerialPort::Parity      parity_;         ///< Паритет. По умолчанию нет
    QSerialPort::StopBits    stop_bits_;      ///< Стоп бит. По умолчанию 1
    QSerialPort::FlowControl flow_control_;   ///< Управление потоком. По умолчанию нет
};

} // namespace Uart
} // namespace Das

#endif // CONFIG_H
