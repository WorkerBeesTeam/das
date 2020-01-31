#include "write_cache_item.h"

namespace Das {

bool Write_Cache_Item::operator ==(Device_Item* other_dev_item)
{
    return dev_item_ == other_dev_item;
}

} // namespace Das
