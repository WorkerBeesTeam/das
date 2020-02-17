#include "save_timer.h"

namespace Das {
namespace DB {

Save_Timer::Save_Timer(uint32_t id, uint32_t interval) :
    id_(id), interval_(interval)
{
}

uint32_t Save_Timer::id() const
{
    return id_;
}

void Save_Timer::set_id(uint32_t id)
{
    id_ = id;
}

uint32_t Save_Timer::interval() const
{
    return interval_;
}

void Save_Timer::set_interval(uint32_t interval)
{
    interval_ = interval;
}

QDataStream& operator>>(QDataStream& ds, Save_Timer& item)
{
    return ds >> item.id_ >> item.interval_;
}

QDataStream& operator<<(QDataStream& ds, const Save_Timer& item)
{
    return ds << item.id() << item.interval();
}

} // namespace DB
} // namespace Das
