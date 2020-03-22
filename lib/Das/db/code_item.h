#ifndef DAS_CODE_ITEM_H
#define DAS_CODE_ITEM_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace DB {

struct DAS_LIBRARY_SHARED_EXPORT Code_Item : public Base_Type
{
    HELPZ_DB_META(Code_Item, "code_item", "ci", 5, DB_A(id), DB_ANS(name), DB_AMN(global_id), DB_AM(text), DB_A(scheme_id))
public:
    Code_Item(uint id = 0, const QString& name = QString(), uint32_t global_id = 0, const QString& text = QString());
    Code_Item(const Code_Item& other) = default;
    Code_Item(Code_Item&& other) = default;
    Code_Item& operator=(const Code_Item& other) = default;
    Code_Item& operator=(Code_Item&& other) = default;
    bool operator==(const Code_Item& other) const;

    uint32_t global_id;
    QString text;
};
QDataStream& operator<<(QDataStream& ds, const Code_Item& item);
QDataStream& operator>>(QDataStream& ds, Code_Item& item);

} // namespace DB

using Code_Item = DB::Code_Item;

} // namespace Das

#endif // DAS_CODE_ITEM_H
