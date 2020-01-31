#include <QDebug>
#include "timerange.h"

namespace Das
{

QTime qTimeFromSecs(uint secs) {
    return QTime(secs / 3600, (secs % 3600) / 60, secs % 60);
}

uint toSecs(const QTime& time) {
    return QTime(0, 0).secsTo( time );
}

//bool nowIsDay(const Prt::DayTimeSecs &dayTime)
//{
//    QTime c = QTime::currentTime();
//    return c >= qTimeFromSecs(dayTime.start()) && c <= qTimeFromSecs(dayTime.end());
//}

TimeRange::TimeRange(uint32_t startSecs, uint32_t endSecs) :
    QObject()
{
    set_range(startSecs, endSecs);
}

TimeRange::TimeRange() :
    TimeRange{ 7 * 3600, 21 * 3600 } {}

TimeRange::TimeRange(const TimeRange &obj) :
    QObject() {
    set_range(obj.m_start, obj.m_end);
}

uint32_t TimeRange::start() const { return m_start; }
uint32_t TimeRange::end() const { return m_end; }

void TimeRange::set_range(uint32_t startSecs, uint32_t endSecs)
{
    m_start = std::min(startSecs, endSecs);
    m_end = std::max(startSecs, endSecs);
}

bool TimeRange::operator !=(const TimeRange &obj) const
{
    return m_start != obj.m_start || m_end != obj.m_end;
}

void TimeRange::operator =(const TimeRange &obj) { m_start = obj.m_start; m_end = obj.m_end; }

bool TimeRange::inRange() const
{
    uint32_t c = toSecs(QTime::currentTime());
    return c >= m_start && c <= m_end;
}

void TimeRange::set_start(uint32_t start)
{
    if (m_start != start)
        m_start = start;
}

void TimeRange::set_end(uint32_t end)
{
    if (m_end != end)
        m_end = end;
}

QDataStream &operator<<(QDataStream &ds, const TimeRange &dayTime)
{
    return ds << dayTime.start() << dayTime.end();
}

QDataStream &operator>>(QDataStream &ds, TimeRange &dayTime)
{
    return ds >> dayTime.m_start >> dayTime.m_end;
}

} // namespace Das

