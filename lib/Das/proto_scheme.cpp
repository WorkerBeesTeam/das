#include <QDebug>

#include "device.h"
#include "proto_scheme.h"

namespace Das {

uint32_t Proto_Scheme::id() const { return id_; }
void Proto_Scheme::set_id(uint32_t id) { if (id_ != id) id_ = id; }

void Proto_Scheme::passEquipment(std::function<void(Device_item_Group *, Device_Item*)> cb_func)
{
    for (const SectionPtr& sct: sections())
    {
        std::vector<uint> items;

        for (Device_item_Group* group: sct->groups())
        {
            for (Device_Item* item: group->items())
                if (item->is_control() &&
                        std::find(items.cbegin(), items.cend(), item->type_id()) == items.cend() )
                {
                    cb_func(group, item);
                    items.push_back( item->type_id() );
                }
        }
    }
}

void Proto_Scheme::setChange(const Log_Value_Item &item) {
    setChangeImpl(item.item_id(), item.raw_value(), item.value());
}

void Proto_Scheme::setChangeImpl(uint32_t item_id, const QVariant &raw_value, const QVariant &display_value)
{
    for (const DevicePtr& dev: devices())
        for (Device_Item* dev_item: dev->items())
            if (dev_item->id() == item_id) {
                dev_item->set_data(raw_value, display_value);
                return;
            }
}

} // namespace Das

