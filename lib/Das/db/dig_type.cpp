#include "dig_type.h"

namespace Das {
namespace Database {

DIG_Type::DIG_Type(uint id, const QString &name, const QString &title, const QString &description) :
    Titled_Type(id, name, title), description_(description)
{
}

QString DIG_Type::description() const { return description_; }
void DIG_Type::set_description(const QString &description) { description_ = description; }

QDataStream &operator<<(QDataStream &ds, const DIG_Type &item)
{
    return ds << static_cast<const Titled_Type&>(item) << item.description();
}

QDataStream &operator>>(QDataStream &ds, DIG_Type &item)
{
    return ds >> static_cast<Titled_Type&>(item) >> item.description_;
}

} // namespace Database
} // namespace Das
