#ifndef DAS_MODBUS_PACK_QUEUE_H
#define DAS_MODBUS_PACK_QUEUE_H

#include <deque>
#include <optional>

#include "modbus_pack_read_manager.h"

namespace Das {
namespace Modbus {

struct Pack_Queue
{
    std::deque<Pack<Write_Cache_Item>> _write;
    std::deque<Pack_Read_Manager> _read;

    bool is_active() const;
    void clear_all();
    void clear(std::optional<int> address = std::optional<int>{});
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_PACK_QUEUE_H
