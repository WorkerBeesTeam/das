#include <QModbusReply>

#include "modbus_pack_queue.h"

namespace Das {
namespace Modbus {

bool Pack_Queue::is_active() const
{
    if (!_write.empty() && _write.front()._reply)
        return true;

    if (!_read.empty())
    {
        int position  = _read.front()._position;
        if (position > -1 && _read.front()._packs.at(position)._reply)
            return true;
    }

    return false;
}

void Pack_Queue::clear_all()
{
    for (auto&& it: _write)
        if (it._reply)
            QObject::disconnect(it._reply, 0, 0, 0);
    _write.clear();
    _read.clear();
}

void Pack_Queue::clear(std::optional<int> address)
{
    for (auto it = _write.begin(); it != _write.end();)
    {
        if (!address.has_value() || it->_server_address == *address)
        {
            if (it->_reply)
                QObject::disconnect(it->_reply, 0, 0, 0);
            it = _write.erase(it);
        }
        else
            ++it;
    }

    for (std::deque<Pack_Read_Manager>::iterator it = _read.begin(); it != _read.end();)
    {
        const Pack<Device_Item*>& pack = it->_packs.front();
        if (!address.has_value() || pack._server_address == *address)
        {
            it = _read.erase(it);
        }
        else
            ++it;
    }
}


} // namespace Modbus
} // namespace Das
