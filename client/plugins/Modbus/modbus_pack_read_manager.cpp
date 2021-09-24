#include <QModbusReply>

#include "modbus_pack_read_manager.h"

namespace Das {
namespace Modbus {

Pack_Read_Manager::Pack_Read_Manager(std::vector<Pack<Device_Item *> > &&packs) :
    _is_connected(true), _position(-1), _packs(std::move(packs))
{
    qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    for (Pack<Device_Item*> &item_pack : _packs)
    {
        for (Device_Item* device_item : item_pack._items)
        {
            _new_values.emplace(device_item, Device::Data_Item{0, timestamp_msecs, {}});
        }
    }
}

Pack_Read_Manager::Pack_Read_Manager(Pack_Read_Manager &&o) :
    _is_connected(std::move(o._is_connected)), _position(std::move(o._position)), _packs(std::move(o._packs)), _new_values(std::move(o._new_values))
{
    o._packs.clear();
    o._new_values.clear();
}

Pack_Read_Manager &Pack_Read_Manager::operator=(Pack_Read_Manager &&o)
{
    _is_connected = std::move(o._is_connected);
    _position = std::move(o._position);
    _packs = std::move(o._packs);
    _new_values = std::move(o._new_values);

    o._packs.clear();
    o._new_values.clear();
    return *this;
}

Pack_Read_Manager::~Pack_Read_Manager()
{
    if (!_is_connected)
    {
        std::vector<Device_Item*> v;
        for (auto it : _packs)
        {
            v.insert(v.end(), it._items.begin(), it._items.end());
        }
        QMetaObject::invokeMethod(_packs.front()._items.front()->device(), "set_device_items_disconnect", Q_ARG(std::vector<Device_Item*>, v));
    }
    else if (_packs.size())
    {
        QMetaObject::invokeMethod(_packs.front()._items.front()->device(), "set_device_items_values",
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>("std::map<Device_Item*,Device::Data_Item>", _new_values), Q_ARG(bool, true));
    }

    while (_packs.size())
    {
        if (_packs.front()._reply)
            QObject::disconnect(_packs.front()._reply, 0, 0, 0);
        _packs.pop_back();
    }
}

} // namespace Modbus
} // namespace Das
