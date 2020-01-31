#include "dig_mode.h"

namespace Das {
namespace Database {

DIG_Mode::DIG_Mode(uint id, const QString &name, const QString &title, uint32_t group_type_id) :
    Titled_Type(id, name, title), group_type_id(group_type_id) {}

QDataStream &operator<<(QDataStream &ds, const DIG_Mode &item)
{
    return ds << static_cast<const Titled_Type&>(item) << item.group_type_id;
}

QDataStream &operator>>(QDataStream &ds, DIG_Mode &item)
{
    return ds >> static_cast<Titled_Type&>(item) >> item.group_type_id;
}

} // namespace Database
} // namespace Das
