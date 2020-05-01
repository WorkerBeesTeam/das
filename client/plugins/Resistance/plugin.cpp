#include <QDebug>
#include <QSettings>
#include <QFile>

#ifndef NO_WIRINGPI
#include <wiringPi.h>
#endif

#include <Helpz/settingshelper.h>

#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/scheme.h>

#include "plugin.h"

namespace Das {
namespace Resistance {

Q_LOGGING_CATEGORY(ResistanceLog, "Resistance")

// ----------

ResistancePlugin::ResistancePlugin() :
    QObject()
{
}

ResistancePlugin::~ResistancePlugin()
{
    stop();
}

void ResistancePlugin::configure(QSettings *settings)
{
    using Helpz::Param;
    conf_ = Helpz::SettingsHelper(
                settings, "Resistance",
                Param<bool>{"IsCounterResult", true},
                Param<int>{"SleepMs", 100},
                Param<int>{"MaxCount", 100000}
    ).obj<Config>();

#ifndef NO_WIRINGPI
    wiringPiSetup();
#endif

    break_ = false;
    thread_ = std::thread(&ResistancePlugin::run, this);
}

bool ResistancePlugin::check(Device* dev)
{
    std::vector<Device_Item*> device_items_disconected;

    bool ok;
    int pin;
    for (Device_Item * item: dev->items())
    {
        pin = item->param("pin").toInt(&ok);
        if (!ok)
        {
            device_items_disconected.push_back(item);
            continue;
        }

        {
            std::lock_guard lock(mutex_);
            if (data_.find(pin) == data_.cend())
            {
                ok = false;
                data_.emplace(pin, item);
            }
        }

        if (!ok)
            cond_.notify_one();
    }

    if (!device_items_disconected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconected));
    }

    return true;
}

void ResistancePlugin::stop()
{
    {
        std::lock_guard lock(mutex_);
        break_ = true;
    }
    cond_.notify_one();

    if (thread_.joinable())
        thread_.join();
}
void ResistancePlugin::write(std::vector<Write_Cache_Item>& /*items*/) {}

void ResistancePlugin::run()
{
    std::unique_lock lock(mutex_, std::defer_lock);

    std::map<Device*, std::map<Device_Item*, Device::Data_Item>> values;
    Device::Data_Item value{0, 0, {}};

    while (true)
    {
        lock.lock();
        cond_.wait(lock, [this, &values](){ return break_ || !data_.empty() || !values.empty(); });

        if (break_) break;

        if (!data_.empty())
        {
            std::pair<int, Device_Item*> item = *data_.begin();
            lock.unlock();

            value.raw_data_ = read_item(item.first);
            value.timestamp_msecs_ = DB::Log_Base_Item::current_timestamp();
            values[item.second->device()].emplace(item.second, value);
        }
        else
        {
            lock.unlock();

            for (const auto& it: values)
            {
                QMetaObject::invokeMethod(it.first, "set_device_items_values", Qt::QueuedConnection,
                                      QArgument<std::map<Device_Item*, Device::Data_Item>>
                                      ("std::map<Device_Item*, Device::Data_Item>", it.second),
                                      Q_ARG(bool, true));
            }
            values.clear();
        }
    }
}

int ResistancePlugin::read_item(int pin)
{
    int count = 0;
#ifndef NO_WIRINGPI
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    std::this_thread::sleep_for(std::chrono::milliseconds(conf_.sleep_ms_));

    auto now = std::chrono::system_clock::now();

    pinMode(pin, INPUT);
    while (digitalRead(pin) == LOW && count < conf_.max_count_)
        ++count;

    if (!conf_.is_counter_result_)
    {
        auto distance = std::chrono::system_clock::now() - now;
        return distance.count();
    }
#endif
    return count;
}

} // namespace Modbus
} // namespace Das
