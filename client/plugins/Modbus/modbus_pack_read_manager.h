#ifndef DAS_MODBUS_PACK_READ_MANAGER_H
#define DAS_MODBUS_PACK_READ_MANAGER_H

#include <Das/device.h>

#include "modbus_pack.h"

namespace Das {
namespace Modbus {

class Pack_Read_Manager
{
public:
    Pack_Read_Manager(const Pack_Read_Manager&) = delete;
    Pack_Read_Manager& operator =(const Pack_Read_Manager&) = delete;
    Pack_Read_Manager(std::vector<Pack<Device_Item*>>&& packs);
    Pack_Read_Manager(Pack_Read_Manager&& o);
    Pack_Read_Manager& operator=(Pack_Read_Manager&& o);
    ~Pack_Read_Manager();

    bool _is_connected;
    int _position; // int becose -1 is default
    std::vector<Pack<Device_Item*>> _packs;
    std::map<Device_Item*, Device::Data_Item> _new_values;
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_PACK_READ_MANAGER_H
