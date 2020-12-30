#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/watchdog.h>

#include <cmath>

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
    auto [stop_at_exit, max_interval, delay_after_check, dev] = Helpz::SettingsHelper{
            settings, "Watchdog",
            Param{"StopAtExit", false},
            Param{"MaxIntervalSecs", 15},
            Param{"DelayAfterCheck", 5},
            Param{"Device", "/dev/watchdog"}
    }();

    _stop_at_exit = stop_at_exit;
    _max_interval = max_interval;
    _delay_after_check = delay_after_check;
    if (_delay_after_check < 1)
        _delay_after_check = 1;
    _dev = dev;
    _reset_cause = get_reset_cause();
}

bool Plugin::check(Device* dev)
{
    if (!dev || dev->items().empty())
        return false;

    if (_dev_handle < 0)
    {
        int interval_sec = std::round(dev->check_interval() / 1000.);
        interval_sec += _delay_after_check;

        open_device(std::min(_max_interval, interval_sec));
    }
    reset_timer();

    Device_Item* dev_item = dev->items().front();

    if (_reset_cause.isValid())
    {
        std::map<Device_Item*, Device::Data_Item> data;
        data.emplace(dev_item, Device::Data_Item{0, DB::Log_Base_Item::current_timestamp(), _reset_cause});

        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*, Device::Data_Item>", data), Q_ARG(bool, true));
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
    if (_dev_handle >= 0)
    {
        if (_stop_at_exit)
            ::write(_dev_handle, "V", 1);
        else
            qCDebug(Log) << "Is not disabled!";

        close(_dev_handle);
        _dev_handle = -1;
    }
}

void Plugin::write(std::vector<Write_Cache_Item>& /*items*/) {}

void Plugin::open_device(int interval_sec)
{
    if (_dev_handle >= 0)
        return;

    _dev_handle = open(qPrintable(_dev), O_RDWR | O_NOCTTY);
    if (_dev_handle < 0)
        qCCritical(Log) << "Couldn't open watchdog device!" << _dev_handle << _dev;
    else
    {
        int timeout = 0;
        ioctl(_dev_handle, WDIOC_SETTIMEOUT, &interval_sec);
        ioctl(_dev_handle, WDIOC_GETTIMEOUT, &timeout);

        if (timeout != interval_sec)
            qCWarning(Log) << "Timeout is" << timeout << "seconds. Not" << interval_sec;
    }
}

void Plugin::reset_timer()
{
    if (ioctl(_dev_handle, WDIOC_KEEPALIVE, 0) != 0)
        stop();
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
