#ifndef DAS_MODBUS_CONFIG_H
#define DAS_MODBUS_CONFIG_H

#include <chrono>

#include <QSerialPort>

#include <Das/device.h>

QT_FORWARD_DECLARE_CLASS(QModbusRtuSerialMaster)
QT_FORWARD_DECLARE_CLASS(QModbusTcpClient)
QT_FORWARD_DECLARE_CLASS(QModbusClient)

namespace Das {
namespace Modbus {

struct Rtu_Config
{
    Rtu_Config& operator=(const Rtu_Config&) = default;
    Rtu_Config& operator=(Rtu_Config&&) = default;
    Rtu_Config(const Rtu_Config&) = default;
    Rtu_Config(Rtu_Config&&) = default;
    Rtu_Config(
            const QString& port_name = QString(),
            QSerialPort::BaudRate speed = QSerialPort::Baud115200,
            QSerialPort::DataBits bits_num = QSerialPort::Data8,
            QSerialPort::Parity parity = QSerialPort::NoParity,
            QSerialPort::StopBits stop_bits = QSerialPort::OneStop,
            QSerialPort::FlowControl flow_control = QSerialPort::NoFlowControl, int frame_delay_mcs = 0);
    Rtu_Config(const QString &address, int frame_delay_mcs = 0);

    QString _name;
    int _baud_rate;   ///< Скорость. По умолчанию 115200
    QSerialPort::DataBits    _data_bits;       ///< Количество бит. По умолчанию 8
    QSerialPort::Parity      _parity;         ///< Паритет. По умолчанию нет
    QSerialPort::StopBits    _stop_bits;       ///< Стоп бит. По умолчанию 1
    QSerialPort::FlowControl _flow_control;   ///< Управление потоком. По умолчанию нет

    int _frame_delay_microseconds;

    /*static QString firstPort() {
        return QSerialPortInfo::availablePorts().count() ? QSerialPortInfo::availablePorts().first().portName() : QString();
    }*/

    static QStringList available_ports();
    static QString getUSBSerial();

    static void set(const Rtu_Config &config, QModbusRtuSerialMaster* device);
};

struct Tcp_Config
{
    Tcp_Config& operator=(const Tcp_Config&) = default;
    Tcp_Config& operator=(Tcp_Config&&) = default;
    Tcp_Config(const Tcp_Config&) = default;
    Tcp_Config(Tcp_Config&&) = default;
    Tcp_Config(const QString& address);

    static void set(const Tcp_Config& config, QModbusTcpClient* device);

    QString _address;
    quint16 _port;
};

struct Config {
    Config& operator=(const Config&) = default;
    Config& operator=(Config&&) = default;
    Config(const Config&) = default;
    Config(Config&&) = default;
    Config(const QString& address, int modbus_timeout = 200, int modbus_number_of_retries = 5,
           int frame_delay_microseconds = 0);
    Config(const QString& address,
         const QString& port_name,
         QSerialPort::BaudRate speed = QSerialPort::Baud115200,
         QSerialPort::DataBits bits_num = QSerialPort::Data8,
         QSerialPort::Parity parity = QSerialPort::NoParity,
         QSerialPort::StopBits stop_bits = QSerialPort::OneStop,
         QSerialPort::FlowControl flow_control = QSerialPort::NoFlowControl,
         int modbus_timeout = 200, int modbus_number_of_retries = 5,
         int frame_delay_microseconds = 0);

    static void set(const Config& config, QModbusClient* device);

    static int32_t line_use_timeout(Device* dev, bool *ok = nullptr);
    static int32_t address(Device* dev, bool *ok = nullptr);
    static int32_t unit(Device_Item* item, bool *ok = nullptr);
    static quint16 count(Device_Item* item, int32_t unit);

    Tcp_Config _tcp;
    Rtu_Config _rtu;

    int _modbus_timeout;
    int _modbus_number_of_retries;
};

} // namespace Modbus
} // namespace Das

#endif // CONFIG_H
