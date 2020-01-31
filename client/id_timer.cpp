#include "id_timer.h"

namespace Das {

ID_Timer::ID_Timer(const Save_Timer &save_timer, QObject *parent) :
    QTimer(parent), Save_Timer(save_timer)
{
    QObject::connect(this, &QTimer::timeout, this, &ID_Timer::on_timer);
}

void ID_Timer::on_timer()
{
    if (Save_Timer::interval())
    {
        setInterval(Save_Timer::interval());
        if (Save_Timer::interval() % 1000 == 0)
            setTimerType(Qt::VeryCoarseTimer);
        Save_Timer::set_interval(0);
    }

    emit timeout(Save_Timer::id());
}

} // namespace Das
