#ifndef DAS_DIG_MODE_H
#define DAS_DIG_MODE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace DB {

struct DAS_LIBRARY_SHARED_EXPORT DIG_Mode_Type : public Titled_Type
{
    HELPZ_DB_META(DIG_Mode_Type, "dig_mode_type", "gmt", DB_A(id), DB_A(name), DB_A(title), DB_AMN(group_type_id), DB_A(scheme_id))
public:
    DIG_Mode_Type(uint id = 0, const QString& name = QString(), const QString& title = QString(), uint32_t group_type_id = 0);
    uint32_t group_type_id;
};

QDataStream& operator<<(QDataStream& ds, const DIG_Mode_Type& item);
QDataStream& operator>>(QDataStream& ds, DIG_Mode_Type& item);

struct DAS_LIBRARY_SHARED_EXPORT DIG_Mode_Type_Manager : public Titled_Type_Manager<DIG_Mode_Type> {};

} // namespace DB

using DIG_Mode_Type = DB::DIG_Mode_Type;
using DIG_Mode_Type_Manager = DB::DIG_Mode_Type_Manager;

} // namespace Das

#endif // DAS_DIG_MODE_H
