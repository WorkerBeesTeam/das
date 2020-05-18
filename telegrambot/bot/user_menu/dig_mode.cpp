#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include <Das/db/dig_mode.h>
#include <Das/db/dig_mode_type.h>

#include "bot/scheme_item.h"
#include "dig_mode.h"

namespace Das {
namespace Bot {
namespace User_Menu {

using namespace Helpz::DB;

std::string DIG_Mode::get_value() const
{
    const uint32_t mode_id = get_mode_id(parameters().scheme_->id(), id());

    QString suffix = "WHERE scheme_id = " + QString::number(parameters().scheme_->parent_id_or_id()) + " AND id = " + QString::number(mode_id);

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.select(db_table<DB::DIG_Mode_Type>(), suffix);
    if (q.next())
        return db_build<DB::DIG_Mode_Type>(q).title().toStdString();

    return ID_Data_Item::get_value();
}

uint32_t DIG_Mode::get_mode_id(uint32_t scheme_id, uint32_t dig_id)
{
    const QString mode_sql = "WHERE scheme_id = " + QString::number(scheme_id) + " AND group_id = " + QString::number(dig_id);
    Base& db = Base::get_thread_local_instance();
    const QVector<DB::DIG_Mode> dig_mode_vect = db_build_list<DB::DIG_Mode>(db, mode_sql);
    if (!dig_mode_vect.empty())
    {
        const DB::DIG_Mode& mode = dig_mode_vect.front();
        return mode.mode_id();
    }

    return 0;
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
