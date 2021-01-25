#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/watchdog.h>

#include <cmath>
#include <condition_variable>
#include <iostream>

#include <QDebug>
#include <QSettings>
#include <QFile>

#include <Helpz/settingshelper.h>

#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/db/device_item_type.h>

#include "plugin.h"

namespace Das {
namespace Watchdog {

Q_LOGGING_CATEGORY(LogDetail, "watchdog.detail", QtWarningMsg)
Q_LOGGING_CATEGORY(Log, "watchdog")

Plugin::Plugin() : QObject() {}

Plugin::~Plugin()
{
    stop();
}

void Plugin::configure(QSettings *settings)
{
    // system("/sbin/modprobe bcm2835_wdt");
    using Helpz::Param;
    auto [stop_at_exit, max_interval, dev_file_name] = Helpz::SettingsHelper{
            settings, "Watchdog",
            Param{"StopAtExit", false},
            Param{"MaxIntervalSecs", 15},
            Param<std::string>{"Device", "/dev/watchdog"}
    }();

    _stop_at_exit = stop_at_exit;
    _max_interval = max_interval;
    _reset_cause = get_reset_cause();

    _file = Helpz::File{dev_file_name};
}

bool Plugin::check(Device* dev)
{
    if (!dev || dev->items().empty())
        return false;

    if (LogDetail().isDebugEnabled())
    {
        static auto now = std::chrono::system_clock::now();
        int interval = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - now).count();
        if (interval > (dev->check_interval() / 1000.))
            qDebug(LogDetail).nospace() << "Reset watchdog " << interval << "s\n";
        now = std::chrono::system_clock::now();
    }

    if (!_file.name().empty())
    {
        if (!_file.is_opened())
            open_device(std::round(dev->check_interval() * 2 / 1000));

        if (_file.is_opened())
            reset_timer();
    }

    Device_Item* dev_item = dev->items().front();

    if (_reset_cause.isValid())
    {
        std::map<Device_Item*, Device::Data_Item> data;
        data.emplace(dev_item, Device::Data_Item{0, DB::Log_Base_Item::current_timestamp(), _reset_cause});

        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*,Device::Data_Item>", data), Q_ARG(bool, true));
    }
    else
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, {dev_item}));
    }

    return true;
}

void Plugin::stop()
{
    if (_file)
    {
        if (_stop_at_exit)
        {
            ::write(_file.descriptor(), "V", 1);

            int flags = WDIOS_DISABLECARD;
            ioctl(_file.descriptor(), WDIOC_SETOPTIONS, &flags);
        }
        else
            qCDebug(Log) << "Is not disabled!";
        _file.close();
    }
}

void Plugin::write(std::vector<Write_Cache_Item>& /*items*/) {}

void Plugin::open_device(int interval_sec)
{
    if (_file.open(Helpz::File::WRITE_ONLY))
    {
        struct watchdog_info info;
        int ret = ioctl(_file.descriptor(), WDIOC_GETSUPPORT, &info);
        if (ret)
        {
            if (!_is_error)
            {
                _is_error = true;
                qCWarning(Log) << "Not supported:" << strerror(errno);
            }
            _file.close();
            return;
        }
        else
        {
            qCDebug(LogDetail) << "identity:" << info.identity
                              << "firmware:" << info.firmware_version
                              << "options:" << info.options;
        }

        if (_is_error)
            _is_error = false;

        int flags = WDIOS_ENABLECARD;
        ioctl(_file.descriptor(), WDIOC_SETOPTIONS, &flags);

        if (LogDetail().isDebugEnabled())
        {
            ret = ioctl(_file.descriptor(), WDIOC_GETBOOTSTATUS, &flags);
            if (!ret)
            {
                qDebug(LogDetail) << "Last boot is caused by:" << ((flags != 0) ? "Watchdog" : "Power-On-Reset");
            }
            else
                qWarning(LogDetail) << strerror(errno);
        }

        if (interval_sec > _max_interval)
            interval_sec = _max_interval;

        int timeout = _max_interval;
        ioctl(_file.descriptor(), WDIOC_SETTIMEOUT, &timeout);
        ioctl(_file.descriptor(), WDIOC_GETTIMEOUT, &timeout);

        ioctl(_file.descriptor(), WDIOC_SETPRETIMEOUT, &interval_sec);
        ioctl(_file.descriptor(), WDIOC_GETPRETIMEOUT, &flags);
        qCDebug(LogDetail) << "Timeout" << timeout << interval_sec << flags;

        if (timeout != _max_interval)
            qCWarning(Log) << "Timeout is" << timeout << "seconds. Not" << interval_sec;
    }
    else if (!_is_error)
    {
        qCCritical(Log) << "Couldn't open watchdog device!" << _file.last_error().c_str() << _file.descriptor() << _file.name().c_str();
        _is_error = true;
    }
}

void Plugin::reset_timer()
{
    if (LogDetail().isDebugEnabled())
    {
        int left = 0;
        int ret = ioctl(_file.descriptor(), WDIOC_GETTIMELEFT, &left);
        if (ret)
            qWarning(LogDetail) << "Get time left error:" << strerror(errno);
        else
            qDebug(LogDetail) << "Left:" << left << "seconds.";
    }

    int res = ioctl(_file.descriptor(), WDIOC_KEEPALIVE, 0);
    if (res)
    {
        qCWarning(Log) << "Can't reset timer:" << _file.last_error().c_str();
        _file.close();
    }
}

QVariant Plugin::get_reset_cause()
{
    int rsts = 0;

    FILE *fp = popen("/opt/vc/bin/vcgencmd get_rsts", "r");
    if (fp)
    {
        fscanf(fp, "get_rsts=%x", &rsts);
        pclose(fp);
    }

    if (rsts & 0x200)
        return 1; // SoftwareReset
    else if (rsts & 0x20)
        return 2; // DogReset
    else if (rsts & 0x1000)
        return 3; // PowerCycle

    qCWarning(Log) << "Unknown reset cause:" << rsts;
    return {}; // Unknown or Error
}

} // namespace Watchdog
} // namespace Das
