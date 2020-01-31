#ifndef DAS_DIG_STATUS_CATEGORY_H
#define DAS_DIG_STATUS_CATEGORY_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace Database {

struct DAS_LIBRARY_SHARED_EXPORT DIG_Status_Category : public Titled_Type
{
    HELPZ_DB_META(DIG_Status_Category, "dig_status_category", "gsc", 5, DB_A(id), DB_A(name), DB_A(title), DB_AM(color), DB_A(scheme_id))
public:
    DIG_Status_Category(uint id = 0, const QString& name = QString(), const QString& title = QString(), const QString& color = QString());
    QString color;
};

QDataStream& operator<<(QDataStream& ds, const DIG_Status_Category& item);
QDataStream& operator>>(QDataStream& ds, DIG_Status_Category& item);

struct DAS_LIBRARY_SHARED_EXPORT DIG_Status_Category_Manager : public Titled_Type_Manager<DIG_Status_Category> {};

} // namespace Database

using DIG_Status_Category = Database::DIG_Status_Category;
using DIG_Status_Category_Manager = Database::DIG_Status_Category_Manager;

} // namespace Das

#endif // DAS_DIG_STATUS_CATEGORY_H
