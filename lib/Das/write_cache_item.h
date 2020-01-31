#ifndef DAS_WRITE_CACHE_ITEM_H
#define DAS_WRITE_CACHE_ITEM_H

#include <QVariant>

namespace Das {

class Device_Item;

struct Write_Cache_Item
{
    uint32_t user_id_;
    Device_Item* dev_item_;
    QVariant raw_data_;

    bool operator ==(Device_Item* other_dev_item);
};

} // namespace Das

#endif // DAS_WRITE_CACHE_ITEM_H
