#ifndef DAS_DATABASE_DEVICE_ITEM_H
#define DAS_DATABASE_DEVICE_ITEM_H

#include <Helpz/db_meta.h>

#include "base_type.h"
#include "device_extra_params.h"

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Device_Item : public Base_Type, public Device_Extra_Params
{
    HELPZ_DB_META(Device_Item, "device_item", "di", DB_A(id), DB_ANS(name), DB_A(type_id), DB_AT(extra),
                  DB_AN(parent_id), DB_A(device_id), DB_AN(group_id), DB_A(scheme_id))
public:
    Device_Item(Device_Item&& o) = default;
    Device_Item(const Device_Item& o) = default;
    Device_Item& operator =(Device_Item&& o) = default;
    Device_Item& operator =(const Device_Item& o) = default;
    Device_Item(uint32_t id = 0, const QString& name = {}, uint32_t type_id = 0, const QVariantList& extra_values = {},
                uint32_t parent_id = 0, uint32_t device_id = 0, uint32_t group_id = 0);

    uint32_t type_id() const;
    void set_type_id(uint32_t type_id);

    uint32_t parent_id() const;
    void set_parent_id(uint32_t parent_id);

    uint32_t device_id() const;
    void set_device_id(uint32_t device_id);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);
private:
    uint32_t type_id_, parent_id_, device_id_, group_id_;

    friend QDataStream &operator>>(QDataStream& ds, Device_Item& dev_item);
};

QDataStream &operator>>(QDataStream& ds, Device_Item& item);
QDataStream &operator<<(QDataStream& ds, const Device_Item& item);

} // namespace DB
} // namespace Das

#endif // DAS_DATABASE_DEVICE_ITEM_H
