#include "dig_status_type.h"

namespace Das {
namespace DB {

DIG_Status_Type::DIG_Status_Type(uint32_t id, const QString &name, const QString &text, uint32_t category_id, uint32_t group_type_id, bool inform) :
    Titled_Type(id, name, text), category_id(category_id), group_type_id(group_type_id), inform(inform)
{
}

QString DIG_Status_Type::text() const { return title(); }
void DIG_Status_Type::set_text(const QString& text) { set_title(text); }

QDataStream &operator<<(QDataStream &ds, const DIG_Status_Type &item)
{
    return ds << static_cast<const Titled_Type&>(item) << item.category_id << item.group_type_id << item.inform;
}

QDataStream &operator>>(QDataStream &ds, DIG_Status_Type &item)
{
    return ds >> static_cast<Titled_Type&>(item) >> item.category_id >> item.group_type_id >> item.inform;
}

} // namespace DB
} // namespace Das
