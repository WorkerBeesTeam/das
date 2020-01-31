#ifndef DAS_DIG_MODE_H
#define DAS_DIG_MODE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace Database {

struct DAS_LIBRARY_SHARED_EXPORT DIG_Mode : public Titled_Type
{
    HELPZ_DB_META(DIG_Mode, "dig_mode", "gm", 5, DB_A(id), DB_A(name), DB_A(title), DB_AMN(group_type_id), DB_A(scheme_id))
public:
    DIG_Mode(uint id = 0, const QString& name = QString(), const QString& title = QString(), uint32_t group_type_id = 0);
    uint32_t group_type_id;
};

QDataStream& operator<<(QDataStream& ds, const DIG_Mode& item);
QDataStream& operator>>(QDataStream& ds, DIG_Mode& item);

struct DAS_LIBRARY_SHARED_EXPORT DIG_Mode_Manager : public Titled_Type_Manager<DIG_Mode> {};

} // namespace Database

using DIG_Mode = Database::DIG_Mode;
using DIG_Mode_Manager = Database::DIG_Mode_Manager;

} // namespace Das

#endif // DAS_DIG_MODE_H
