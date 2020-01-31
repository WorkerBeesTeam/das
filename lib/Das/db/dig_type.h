#ifndef DAS_DIG_TYPE_H
#define DAS_DIG_TYPE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT DIG_Type : public Titled_Type
{
    HELPZ_DB_META(DIG_Type, "dig_type", "gt", 5, DB_A(id), DB_A(name), DB_A(title), DB_ANS(description), DB_A(scheme_id))
public:
    DIG_Type(uint id = 0, const QString& name = QString(), const QString& title = QString(),
                  const QString& description = QString());

    QString description() const;
    void set_description(const QString &description);

private:
    QString description_;

    friend QDataStream& operator>>(QDataStream& ds, DIG_Type& item);
};

QDataStream& operator<<(QDataStream& ds, const DIG_Type& item);
QDataStream& operator>>(QDataStream& ds, DIG_Type& item);

struct DAS_LIBRARY_SHARED_EXPORT DIG_Type_Manager : public Titled_Type_Manager<DIG_Type> {};

} // namespace Database

using DIG_Type = Database::DIG_Type;
using DIG_Type_Manager = Database::DIG_Type_Manager;

} // namespace Das

#endif // DAS_DIG_TYPE_H
