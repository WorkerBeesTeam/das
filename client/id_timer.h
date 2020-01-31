#ifndef DAS_ID_TIMER_H
#define DAS_ID_TIMER_H

#include <QTimer>

#include <Das/db/save_timer.h>

namespace Das
{

class ID_Timer: public QTimer, Save_Timer
{
    Q_OBJECT
public:
     explicit ID_Timer(const Save_Timer &save_timer, QObject *parent = nullptr);

signals:
    void timeout(uint32_t id);
private slots:
    void on_timer();
private:
    int timer_id_;
};

} // namespace Das

#endif // DAS_ID_TIMER_H
