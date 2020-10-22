#ifndef DAS_SERVER_LOG_MANAGER_H
#define DAS_SERVER_LOG_MANAGER_H

#include "log_saver/log_saver_controller.h"

namespace Das {
namespace Server {
namespace Log_Saver {

class Manager : public Controller
{
    static Manager* _instance;
public:
    static Manager* instance();

    Manager(size_t thread_count = 5, size_t max_pack_size = 100, chrono::seconds time_in_cache = 15s);
    ~Manager();

    void set_devitem_values(QVector<Device_Item_Value> &&data, uint32_t scheme_id);
    QVector<Device_Item_Value> get_devitem_values(uint32_t scheme_id);

    void set_statuses(QVector<DIG_Status>&& data, uint32_t scheme_id);
    std::set<DIG_Status> get_statuses(uint32_t scheme_id);
};

Manager* instance();

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_MANAGER_H
