#ifndef DAS_Device_ITEM_TYPE_H
#define DAS_Device_ITEM_TYPE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Device_Item_Type : public Titled_Type
{
    HELPZ_DB_META(Device_Item_Type, "device_item_type", "dit", 9, DB_A(id), DB_A(name), DB_A(title),
                  DB_AM(group_type_id), DB_AMN(sign_id), DB_AM(register_type), DB_AM(save_algorithm),
                  DB_AMN(save_timer_id), DB_A(scheme_id))
public:
    enum Register_Type {
        RT_INVALID,
        RT_DISCRETE_INPUTS,
        RT_COILS,
        RT_INPUT_REGISTERS,
        RT_HOLDING_REGISTERS,
        RT_FILE,
        RT_SIMPLE_BUTTON,
        RT_VIDEO_STREAM,
    };

    enum Save_Algorithm {
        SA_INVALID,
        SA_OFF,
        SA_IMMEDIATELY,
        SA_BY_TIMER,
        SA_BY_TIMER_ANY_CASE,
    };

    Device_Item_Type(uint32_t id = 0, const QString& name = QString(), const QString& title = QString(), uint32_t group_type_id = 0,
             uint32_t sign_id = 0, uint8_t register_type = RT_INVALID, uint8_t save_algorithm = SA_INVALID, uint32_t save_timer_id = 0);
    Device_Item_Type(Device_Item_Type&& o) = default;
    Device_Item_Type(const Device_Item_Type& o) = default;
    Device_Item_Type& operator=(Device_Item_Type&& o) = default;
    Device_Item_Type& operator=(const Device_Item_Type& o) = default;

    uint32_t group_type_id = 0;
    uint32_t sign_id = 0;
    uint8_t register_type = RT_INVALID;
    uint8_t save_algorithm = SA_INVALID;
    uint32_t save_timer_id = 0;
};

QDataStream& operator>>(QDataStream& ds, Device_Item_Type& item);
QDataStream& operator<<(QDataStream& ds, const Device_Item_Type& item);

struct DAS_LIBRARY_SHARED_EXPORT Device_Item_Type_Manager : public Titled_Type_Manager<Device_Item_Type>
{
    uint32_t group_type_id(uint type_id) const;
    uint32_t sign_id(uint type_id) const;
    uchar register_type(uint type_id) const;
    uchar save_algorithm(uint type_id) const;
    uint32_t save_timer_id(uint type_id) const;
};

} // namespace DB

using Device_Item_Type = DB::Device_Item_Type;
using Device_Item_Type_Manager = DB::Device_Item_Type_Manager;

} // namespace Das

#endif // DAS_Device_ITEM_TYPE_H
