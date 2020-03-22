#include "dig_mode_type.h"

namespace Das {
namespace DB {

DIG_Mode_Type::DIG_Mode_Type(uint id, const QString &name, const QString &title, uint32_t group_type_id) :
    Titled_Type(id, name, title), group_type_id(group_type_id) {}

QDataStream &operator<<(QDataStream &ds, const DIG_Mode_Type &item)
{
    return ds << static_cast<const Titled_Type&>(item) << item.group_type_id;
}

QDataStream &operator>>(QDataStream &ds, DIG_Mode_Type &item)
{
    return ds >> static_cast<Titled_Type&>(item) >> item.group_type_id;
}

} // namespace DB
} // namespace Das
