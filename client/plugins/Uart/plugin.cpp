#include <QSettings>

#include <Helpz/settingshelper.h>

#include "plugin.h"

namespace Das {

Uart_Plugin::Uart_Plugin() :
    QObject()
{}

Uart_Plugin::~Uart_Plugin()
{
    if (_thread.joinable())
    {
        _thread.stop();
        _thread.join();
    }
}

void Uart_Plugin::configure(QSettings *settings)
{
    using Helpz::Param;

    Uart::Config& config = Uart::Config::instance();

    config = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Param<QString>,Param<QSerialPort::BaudRate>,Param<QSerialPort::DataBits>,
                            Param<QSerialPort::Parity>,Param<QSerialPort::StopBits>,
                            Param<QSerialPort::FlowControl>>
        #endif
            (
                settings, "Uart",
                Param<QString>{"Port", config._name},
                Param<int>{"BaudRate", config._baud_rate},
                Param<QSerialPort::DataBits>{"DataBits", config._data_bits},
                Param<QSerialPort::Parity>{"Parity", config._parity},
                Param<QSerialPort::StopBits>{"StopBits", config._stop_bits},
                Param<QSerialPort::FlowControl>{"FlowControl", config._flow_control},
                Param<int>{"MinReadTimeout", config._min_read_timeout},
                Param<int>{"WriteTimeout", config._write_timeout},
                Param<int>{"RepeatCount", config._repeat_count},
                Param<std::string>{"LuaScript", config._lua_script_file},
                Param<bool>{"LuaUseLibs", config._lua_use_libs}
    ).obj<Uart::Config>();

    _thread.start();
}

bool Uart_Plugin::check(Device* dev)
{
    return _thread.check(dev);
}

void Uart_Plugin::stop()
{
    _thread.stop();
}

void Uart_Plugin::write(Device */*dev*/, std::vector<Write_Cache_Item>& items)
{
    _thread.write(items);
}

} // namespace Das
