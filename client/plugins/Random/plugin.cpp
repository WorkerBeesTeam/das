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

void RandomPlugin::configure(QSettings */*settings*/, Scheme */*scheme*/)
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

    for (Device_Item* item: dev->items())
    {
        if (writed_list_.find(item->id()) != writed_list_.cend())
            continue;
        switch (static_cast<Device_Item_Type::Register_Type>(item->register_type())) {
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
            break;
        }

        QMetaObject::invokeMethod(item, "set_raw_value", Qt::QueuedConnection, Q_ARG(const QVariant&, value));
    }

    return true;
}

void RandomPlugin::stop() {}

void RandomPlugin::write(std::vector<Write_Cache_Item>& items)
{
    for (const Write_Cache_Item& item: items)
    {
        QMetaObject::invokeMethod(item.dev_item_, "set_raw_value", Qt::QueuedConnection,
                                  Q_ARG(const QVariant&, item.raw_data_), Q_ARG(bool, false), Q_ARG(uint32_t, item.user_id_));
        writed_list_.insert(item.dev_item_->id());
    }
}

int RandomPlugin::random(int min, int max) const {
    return min + (qrand() % static_cast<int>(max - min + 1));
}

} // namespace Random
} // namespace Das
