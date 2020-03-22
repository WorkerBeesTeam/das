#include "dig_status_category.h"

namespace Das {
namespace DB {

DIG_Status_Category::DIG_Status_Category(uint id, const QString &name, const QString &title, const QString &color) :
    Titled_Type(id, name, title), color(color)
{
}

QDataStream &operator<<(QDataStream &ds, const DIG_Status_Category &item)
{
    return ds << static_cast<const Titled_Type&>(item) << item.color;
}

QDataStream &operator>>(QDataStream &ds, DIG_Status_Category &item)
{
    return ds >> static_cast<Titled_Type&>(item) >> item.color;
}

} // namespace DB
} // namespace Das
