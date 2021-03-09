#include <QDebug>
#include <QSettings>
#include <QFile>

#include <Helpz/settingshelper.h>
#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/db/device_item_type.h>

#include "plugin.h"

namespace Das {
namespace Random {

Q_LOGGING_CATEGORY(RandomLog, "random")

RandomPlugin::RandomPlugin() : QObject() {}

void RandomPlugin::configure(QSettings */*settings*/)
{
    using Helpz::Param;
//    auto [interval] = Helpz::SettingsHelper(
//                settings, "Random",
//                Param{"interval", 350},
//    ).unique_ptr<Conf>();

    qsrand(time(NULL));
}

bool RandomPlugin::check(Device* dev)
{
    if (!dev)
        return false;

    std::vector<Device_Item*> device_items_disconected;
    std::map<Device_Item*, Device::Data_Item> device_items_values;
    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    for (Device_Item* item: dev->items())
    {
        if (writed_list_.find(item->id()) != writed_list_.cend())
            continue;

        switch (static_cast<Device_Item_Type::Register_Type>(item->register_type()))
        {
        case Device_Item_Type::RT_DISCRETE_INPUTS:
        case Device_Item_Type::RT_COILS:
            value = random(-32767, 32768) > 0;
            break;
        case Device_Item_Type::RT_INPUT_REGISTERS:
        case Device_Item_Type::RT_HOLDING_REGISTERS:
            value = random(-32767, 32768);
            break;
        default:
            value.clear();
            device_items_disconected.push_back(item);
            break;
        }

        if (value.isValid())
        {
            Device::Data_Item data_item{0, timestamp_msecs, value};
            device_items_values.emplace(item, std::move(data_item));
        }
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*, Device::Data_Item>", device_items_values), Q_ARG(bool, true));
    }

    if (!device_items_disconected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconected));
    }

    return true;
}

void RandomPlugin::stop() {}

void RandomPlugin::write(Device* dev, std::vector<Write_Cache_Item>& items)
{
    std::map<Device_Item*, Device::Data_Item> device_items_values;
    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    for (const Write_Cache_Item& item: items)
    {
        Device::Data_Item data_item{item.user_id_, timestamp_msecs, item.raw_data_};
        device_items_values.emplace(item.dev_item_, std::move(data_item));

        writed_list_.insert(item.dev_item_->id());
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*, Device::Data_Item>", device_items_values), Q_ARG(bool, true));
    }
}

int RandomPlugin::random(int min, int max) const
{
    return min + (qrand() % static_cast<int>(max - min + 1));
}

} // namespace Random
} // namespace Das
