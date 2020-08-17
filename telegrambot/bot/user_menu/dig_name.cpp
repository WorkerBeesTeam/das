#include <Helpz/db_base.h>

#include "bot/scheme_item.h"
#include "dig_name.h"

namespace Das {
namespace Bot {
namespace User_Menu {

using namespace Helpz::DB;

std::string DIG_Name::get_value() const
{
    const QString sql =
            "SELECT IF(dig.title IS NULL OR dig.title = '', dt.title, dig.title) as title "
            "FROM das_device_item_group dig "
            "LEFT JOIN das_dig_type dt ON dt.id = dig.type_id "
            "WHERE dig." + parameters().scheme_->ids_to_sql()
            + " AND dig.id = " + QString::number(id());

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql);
    if (q.next())
        return q.value(0).toString().toStdString();
    return ID_Data_Item::get_value();
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
