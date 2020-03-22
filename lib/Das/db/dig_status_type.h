#ifndef DAS_DIG_STATUS_TYPE_H
#define DAS_DIG_STATUS_TYPE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace DB {

struct DAS_LIBRARY_SHARED_EXPORT DIG_Status_Type : public Titled_Type
{
    HELPZ_DB_META(DIG_Status_Type, "dig_status_type", "hs", 7, DB_A(id), DB_A(name), DB_A(text), DB_AM(category_id),
                  DB_AMN(group_type_id), DB_AM(inform), DB_A(scheme_id))
public:
    DIG_Status_Type(uint32_t id = 0, const QString& name = QString(), const QString& text = QString(), uint32_t category_id = 0,
                uint32_t group_type_id = 0, bool inform = true);

    QString text() const;
    void set_text(const QString& text);

    uint32_t category_id;
    uint32_t group_type_id;
    bool inform;
};

QDataStream& operator<<(QDataStream& ds, const DIG_Status_Type& item);
QDataStream& operator>>(QDataStream& ds, DIG_Status_Type& item);

struct DAS_LIBRARY_SHARED_EXPORT Status_Manager : public Titled_Type_Manager<DIG_Status_Type> {};

} // namespace DB

using DIG_Status_Type = DB::DIG_Status_Type;
using Status_Manager = DB::Status_Manager;

} // namespace Das

#endif // DAS_DIG_STATUS_TYPE_H
