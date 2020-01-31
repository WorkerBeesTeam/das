#ifndef DAS_TIMERANGE_H
#define DAS_TIMERANGE_H

#include <QTime>
#include <QObject>
#include <QDataStream>

#include "daslib_global.h"

namespace Das
{

QTime DAS_LIBRARY_SHARED_EXPORT qTimeFromSecs(uint secs);
uint DAS_LIBRARY_SHARED_EXPORT toSecs(const QTime &time);
//bool nowIsDay(const Prt::DayTimeSecs& dayTime);

class DAS_LIBRARY_SHARED_EXPORT TimeRange : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint32_t start READ start WRITE set_start)
    Q_PROPERTY(uint32_t end READ end WRITE set_end)
public:
    TimeRange(uint32_t startSecs, uint32_t endSecs);
    TimeRange();
    TimeRange(const TimeRange& obj);
//    TimeRange(const Prt::DayTimeSecs& obj);
    TimeRange(TimeRange&& obj) = default;

    uint32_t start() const;
    uint32_t end() const;

    void set_range(uint32_t startSecs, uint32_t endSecs);
//    void set_range(const Prt::DayTimeSecs &range);

    bool operator !=(const TimeRange &obj) const;
    void operator =(const TimeRange &obj);
//    void operator =(const Prt::DayTimeSecs &obj);

public slots:
    bool inRange() const;

    void set_start(uint32_t start);
    void set_end(uint32_t end);

private:
    uint32_t m_start, m_end;
    friend QDataStream& operator>>(QDataStream& ds, Das::TimeRange& dayTime);
};

QDataStream& operator<<(QDataStream& ds, const TimeRange& dayTime);
QDataStream& operator>>(QDataStream& ds, TimeRange& dayTime);

} // namespace Das

Q_DECLARE_METATYPE(Das::TimeRange)

#endif // DAS_TIMERANGE_H
