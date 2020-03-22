#include "code_item.h"

namespace Das {
namespace DB {

Code_Item::Code_Item(uint id, const QString &name, uint32_t global_id, const QString &text) :
    Base_Type(id, name), global_id(global_id), text(text) {}

bool Code_Item::operator==(const Code_Item &other) const
{
    return id() == other.id() && name() == other.name() && global_id == other.global_id && text == other.text;
}

QDataStream &operator<<(QDataStream &ds, const Code_Item &item)
{
    return ds << static_cast<const Base_Type&>(item) << item.global_id << item.text;
}

QDataStream &operator>>(QDataStream &ds, Code_Item &item)
{
    return ds >> static_cast<Base_Type&>(item) >> item.global_id >> item.text;
}

} // namespace DB
} // namespace Das
