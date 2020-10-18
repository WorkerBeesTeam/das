#ifndef DAS_SERVER_LOG_MANAGER_H
#define DAS_SERVER_LOG_MANAGER_H

#include "log_saver/log_saver_controller.h"

namespace Das {
namespace Server {

class Log_Manager
{
    static Log_Manager* _instance;
public:
    static Log_Manager* instance();

    Log_Manager();
    ~Log_Manager();

    void stop();

    Log_Saver::Controller& ctrl();

    void set_devitem_values(QVector<Log_Value_Item>&& data, uint32_t scheme_id);
    QVector<Device_Item_Value> get_devitem_values(uint32_t scheme_id);

    void set_statuses(QVector<Log_Status_Item>&& data, uint32_t scheme_id);
    std::set<DIG_Status> get_statuses(uint32_t scheme_id);
private:
    Log_Saver::Controller _ctrl;
};

} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_MANAGER_H
