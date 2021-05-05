#ifndef DAS_LOG_SAVE_CONTROLLER_H
#define DAS_LOG_SAVE_CONTROLLER_H

#include <Helpz/db_thread.h>

#include <Das/db/device_item_value.h>
#include <Das/db/dig_param_value.h>

#include "pack_sender.h"

namespace Das {

class Worker;
class Scripted_Scheme;
class ID_Timer;

using namespace std::chrono_literals;

struct Log_Save_Config
{
    static Log_Save_Config& instance()
    {
        static Log_Save_Config conf;
        return conf;
    }

    std::chrono::milliseconds _dev_item_value = 5000ms;
    std::chrono::milliseconds _dev_item_value_force = 500ms;

    std::chrono::milliseconds _log_value = 5000ms;
    std::chrono::milliseconds _log_value_force = 500ms;

    std::chrono::milliseconds _log_event = 1000ms;
    int _log_event_force_size = 100;

    std::chrono::milliseconds _log_param = 5ms;
    std::chrono::milliseconds _log_status = 250ms;
    std::chrono::milliseconds _log_mode = 0ms;
};

class Log_Save_Controller : public QObject
{
    Q_OBJECT
public:
    Log_Save_Controller(Worker* worker);
    ~Log_Save_Controller();

signals:
public slots:
    QVector<Device_Item_Value> get_unsaved_values() const;

private slots:
    void add_log_value_item(Log_Value_Item item);
    void add_log_event_item(const Log_Event_Item& item);
    void add_log_param_item(const DIG_Param_Value& param_value);
    void add_log_status_item(const DIG_Status& status);
    void add_log_mode_item(const DIG_Mode& mode);

    void save_device_item_values();
    void send_log_value_pack();
    void send_log_event_pack();
    void send_log_param_pack();
    void send_log_status_pack();
    void send_log_mode_pack();
private:
    void save_dig_param_values(const QVector<Log_Param_Item> &pack);
    void stop();
    void process_user_timer(uint32_t timer_id);

    const Log_Save_Config& conf() const;

    Scripted_Scheme* prj_;
    Worker* worker_;

    std::vector<ID_Timer*> timers_list_;
    std::map<uint32_t, std::pair<QVariant, QVariant>> timer_cached_values_;

    Pack_Sender<Log_Value_Item> _value_pack;
    Pack_Sender<Log_Event_Item> _event_pack;
    Pack_Sender<Log_Param_Item> _param_pack;
    Pack_Sender<Log_Status_Item> _status_pack;
    Pack_Sender<Log_Mode_Item> _mode_pack;

    std::map<uint32_t, Device_Item_Value> cached_device_item_values_;
    QTimer device_item_values_timer_;
};

} // namespace Das

#endif // DAS_LOG_SAVE_CONTROLLER_H
