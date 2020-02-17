#ifndef DAS_LOG_VALUE_SAVE_TIMER_H
#define DAS_LOG_VALUE_SAVE_TIMER_H

#include <QTimer>

#include <Helpz/db_thread.h>

#include <Das/log/log_type.h>
#include <Das/log/log_pack.h>
#include <Das/db/device_item_value.h>
#include <Das/db/dig_param_value.h>

namespace Das {

class Worker;
class Scripted_Scheme;
class ID_Timer;

class Log_Value_Save_Timer : public QObject
{
    Q_OBJECT
public:
    Log_Value_Save_Timer(Worker* worker);
    ~Log_Value_Save_Timer();

signals:
//    void change(const Log_Value_Item& item, bool immediately);
public slots:
    QVector<Device_Item_Value> get_unsaved_values() const;

    void add_log_value_item(const Log_Value_Item& item);
    void add_log_event_item(const Log_Event_Item& item);
    void add_param_value(const DIG_Param_Value& param_value);

    void status_changed(const DIG_Status& status);
    void dig_mode_changed(const DIG_Mode& mode);
private slots:
    void save_item_values();
    void send_value_pack();
    void send_event_pack();
    void send_param_pack();
private:
    void save_dig_param_values(std::shared_ptr<QVector<DIG_Param_Value> > pack);
    void stop();
    void process_items(uint32_t timer_id);

    template<typename T>
    void send(Log_Type_Wrapper log_type, std::shared_ptr<QVector<T>> pack);
    template<typename T>
    bool save_to_db(const QVector<T>& pack);

    Scripted_Scheme* prj_;
    Worker* worker_;

    std::vector<ID_Timer*> timers_list_;
    std::map<uint32_t, std::pair<QVariant, QVariant>> cached_values_;

    QVector<Log_Value_Item> value_pack_;
    QVector<Log_Event_Item> event_pack_;
    QVector<DIG_Param_Value> param_pack_;

    std::map<uint32_t, Device_Item_Value> waited_item_values_;
    QTimer item_values_timer_, value_pack_timer_, event_pack_timer_, param_values_timer_;
};

} // namespace Das

#endif // DAS_LOG_VALUE_SAVE_TIMER_H
