#ifndef DAS_LIGHT_INDICATOR_H
#define DAS_LIGHT_INDICATOR_H

#include <QWidget>

namespace Das {

class Light_Indicator : public QWidget
{
    Q_OBJECT
public:
    explicit Light_Indicator(QWidget *parent = nullptr);

signals:
public slots:
    void toggle();
    void set_state(bool state);
    void on();
    void off();
private:
    void paintEvent(QPaintEvent *) override;
    bool state_;
};

} // namespace Das

#endif // DAS_LIGHT_INDICATOR_H
