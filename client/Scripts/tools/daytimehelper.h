#ifndef DAYTIMEHELPER_H
#define DAYTIMEHELPER_H

#include <QTimer>

#include "automationhelper.h"

namespace Das {

class DayTimeHelper : public QObject {
    Q_OBJECT
public:
    DayTimeHelper(Scheme* prj, QObject* parent = nullptr);

    void stop();
signals:
    void onDayPartChanged(Section*, bool is_day);
public slots:
    void init();

private slots:
    void onTimer();

private:
    Scheme* prj;
    QTimer m_timer;
};

} // namespace Das

#endif // DAYTIMEHELPER_H
