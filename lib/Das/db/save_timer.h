#ifndef DAS_DATABASE_SAVE_TIMER_H
#define DAS_DATABASE_SAVE_TIMER_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Save_Timer : public Schemed_Model
{
    HELPZ_DB_META(Save_Timer, "save_timer", "st", 3, DB_A(id), DB_A(interval), DB_A(scheme_id))
public:
    Save_Timer(Save_Timer&& other) = default;
    Save_Timer(const Save_Timer& other) = default;
    Save_Timer& operator=(Save_Timer&& other) = default;
    Save_Timer& operator=(const Save_Timer& other) = default;
    Save_Timer(uint32_t id = 0, uint32_t interval = 1500);

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t interval() const;
    void set_interval(uint32_t interval);

private:
    uint32_t id_, interval_;

    friend QDataStream &operator>>(QDataStream &ds, Save_Timer& item);
};

QDataStream &operator>>(QDataStream &ds, Save_Timer& item);
QDataStream &operator<<(QDataStream &ds, const Save_Timer& item);

} // namespace DB

using Save_Timer = DB::Save_Timer;

} // namespace Das

#endif // DAS_DATABASE_SAVE_TIMER_H
