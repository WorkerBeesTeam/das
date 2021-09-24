
#include <vector>
#include <queue>
#include <iterator>
#include <type_traits>

#include <QDebug>
#include <QSettings>
#include <QFile>

#ifdef QT_DEBUG
#include <QtDBus>
#endif

#include <Helpz/settingshelper.h>
#include <Helpz/simplethread.h>

#include <Das/db/device_item_type.h>
#include <Das/device.h>

#include "modbus_thread.h"
#include "modbus_pack_queue.h"
#include "modbus_pack_builder.h"
#include "modbus_pack_read_manager.h"
#include "modbus_plugin_base.h"

namespace Das {
namespace Modbus {

Modbus_Plugin_Base::Modbus_Plugin_Base() :
    base_config_(QString{}, QString{}),
    b_break(false)
{
}

Modbus_Plugin_Base::~Modbus_Plugin_Base()
{
    stop();
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
            Param<std::chrono::milliseconds>{"LineUseTimeout", std::chrono::milliseconds(50)}
    )();
    _auto_port = auto_port;


#if defined(QT_DEBUG) && defined(Q_OS_UNIX)
    if (QDBusConnection::sessionBus().isConnected())
    {
        QDBusInterface iface("ru.deviceaccess.Das.Emulator", "/", "", QDBusConnection::sessionBus());
        if (iface.isValid())
        {
            QDBusReply<QString> tty_path = iface.call("ttyPath");
            if (tty_path.isValid())
                base_config_._rtu._name = tty_path.value(); // "/dev/ttyUSB0";
        }
        else
        {
            QString overTCP = "/home/kirill/vmodem0";
            base_config_._rtu._name = QFile::exists(overTCP) ? overTCP : base_config_._rtu._name;//"/dev/pts/10";
        }
    }
#endif
}

bool Modbus_Plugin_Base::check(Device* dev)
{
    clear_break_flag();

    Controller* ctrl = controller_by_device(dev);
    return ctrl->read(dev);
}

void Modbus_Plugin_Base::stop()
{
    b_break = true;

    for (auto&& it: _thread_map)
        delete it.first;
    _thread_map.clear();
}

void Modbus_Plugin_Base::write(Device *dev, std::vector<Write_Cache_Item>& items)
{
    Controller* ctrl = controller_by_device(dev);
    ctrl->write(items);
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
    QObject* obj = this;
    if (QThread::currentThread() == thread())
        obj = controller_by_device(dev);

    QMetaObject::invokeMethod(obj, "write_multivalue", Qt::QueuedConnection,
                              Q_ARG(Device*, dev), Q_ARG(int, server_address), Q_ARG(int, start_address),
                              Q_ARG(QVariantList, data), Q_ARG(bool, is_coils), Q_ARG(bool, silent));
}


void Modbus_Plugin_Base::print_cached(QModbusClient *modbus, int server_address, QModbusDataUnit::RegisterType register_type,
                                      QModbusDevice::Error value, const QString& text)
{
    Status_Holder request_pair{server_address, register_type, reinterpret_cast<qintptr>(modbus)};
    auto status_it = dev_status_cache_.find(request_pair);

    if (status_it == dev_status_cache_.end() || status_it->second != value)
    {
        qCWarning(Log).noquote() << text;

        if (status_it == dev_status_cache_.end())
            dev_status_cache_[request_pair] = value;
        else
            status_it->second = value;
    }
}

Controller* Modbus_Plugin_Base::controller_by_device(Device *dev)
{
    for (auto it = _thread_map.cbegin(); it != _thread_map.cend(); ++it)
        if (std::find(it->second.cbegin(), it->second.cend(), dev) != it->second.cend())
            return it->first->controller();

    Config config = dev_config(dev, /*simple=*/true);

    auto it = _thread_map.begin();
    for (; it != _thread_map.end(); ++it)
    {
        if (!config._tcp._address.isEmpty())
        {
            if (config._tcp._address == it->first->controller()->config()._tcp._address
                && config._tcp._port == it->first->controller()->config()._tcp._port)
                break;
        }
        else if (config._rtu._name == it->first->controller()->config()._rtu._name)
            break;
    }

    Controller* ctrl;
    if (it != _thread_map.end())
    {
        it->second.push_back(dev);
        ctrl = it->first->controller();
    }
    else
    {
        config = dev_config(dev);
        auto thread = new Thread(config, this);

        _thread_map.emplace(thread, std::vector<Device*>{dev});
        ctrl = thread->controller();
    }

    int line_use_timeout = Config::line_use_timeout(dev);
    QMetaObject::invokeMethod(ctrl, "set_line_use_timeout", Q_ARG(int, line_use_timeout));
    return ctrl;
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

} // namespace Modbus
} // namespace Das
