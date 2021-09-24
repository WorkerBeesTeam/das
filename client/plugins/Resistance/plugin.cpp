
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
    _conf = Helpz::SettingsHelper(
                settings, "Resistance",
                Param<bool>{"IsZeroDisconnected", true},
                Param<int>{"SleepMs", 100},
                Param<int>{"MaxCount", 10000000},
                Param<int>{"SumFor", 3},
                Param<int>{"DevideBy", 100}
    ).obj<Config>();

#ifdef NO_WIRINGPI
    srand(time(nullptr));
#else
    wiringPiSetup();
#endif

    _break = false;
    _thread = std::thread(&ResistancePlugin::run, this);
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
            std::lock_guard lock(_mutex);
            if (_data.find(pin) == _data.cend())
            {
                ok = false;
                _data.emplace(pin, item);
            }
        }

        if (!ok)
            _cond.notify_one();
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
        std::lock_guard lock(_mutex);
        _break = true;
    }
    _cond.notify_one();

    if (_thread.joinable())
        _thread.join();
}
void ResistancePlugin::write(Device */*dev*/, std::vector<Write_Cache_Item>& /*items*/) {}

void ResistancePlugin::run()
{
    struct Device_Data_Pack
    {
        std::map<Device_Item*, Device::Data_Item> values_;
        std::vector<Device_Item*> disconnected_vect_;
    };

    std::map<Device*, Device_Data_Pack> devices_data_pack;
    Device::Data_Item data_item{0, 0, {}};
    int value, value_sum, sum_for;

    std::unique_lock lock(_mutex, std::defer_lock);

    while (true)
    {
        lock.lock();
        _cond.wait(lock, [this, &devices_data_pack](){ return _break || !_data.empty() || !devices_data_pack.empty(); });

        if (_break) break;

        if (!_data.empty())
        {
            std::pair<int, Device_Item*> item = *_data.begin();
            lock.unlock();

            Device_Data_Pack& device_data_pack = devices_data_pack[item.second->device()];

            value_sum = sum_for = 0;
            for (int i = 0; i < _conf.sum_for_; ++i)
            {
                value = read_item(item.first);
                if (value != 0)
                {
                    value_sum += value;
                    ++sum_for;
                }
            }

            if (_conf.is_zero_disconnected_ && sum_for == 0)
            {
                device_data_pack.disconnected_vect_.push_back(item.second);
            }
            else
            {
                if (sum_for != 0)
                    value_sum /= sum_for;
                if (_conf.devide_by_ != 0)
                    value_sum /= _conf.devide_by_;

                if (_conf.is_zero_disconnected_ && value_sum == 0)
                    device_data_pack.disconnected_vect_.push_back(item.second);
                else
                {
                    data_item.raw_data_ = value_sum;
                    data_item.timestamp_msecs_ = DB::Log_Base_Item::current_timestamp();
                    device_data_pack.values_.emplace(item.second, data_item);
                }
            }

            std::lock_guard erase_lock(_mutex);
            _data.erase(_data.begin());
        }
        else
        {
            lock.unlock();

            for (const auto& it: devices_data_pack)
            {
                if (!it.second.values_.empty())
                    QMetaObject::invokeMethod(it.first, "set_device_items_values", Qt::QueuedConnection,
                                      QArgument<std::map<Device_Item*, Device::Data_Item>>
                                      ("std::map<Device_Item*, Device::Data_Item>", it.second.values_),
                                      Q_ARG(bool, true));

                if (!it.second.disconnected_vect_.empty())
                    QMetaObject::invokeMethod(it.first, "set_device_items_disconnect", Qt::QueuedConnection,
                                              Q_ARG(std::vector<Device_Item*>, it.second.disconnected_vect_));
            }
            devices_data_pack.clear();
        }
    }
}

int ResistancePlugin::read_item(int pin)
{
    int count = 0;
#ifdef NO_WIRINGPI
    count = rand() % _conf.max_count_;
#else
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    std::this_thread::sleep_for(std::chrono::milliseconds(_conf.sleep_ms_));

    pinMode(pin, INPUT);

    while (digitalRead(pin) == LOW && count < _conf.max_count_)
        ++count;

    if (count == _conf.max_count_)
        count = 0;
#endif
    return count;
}

} // namespace Resistance
} // namespace Das
