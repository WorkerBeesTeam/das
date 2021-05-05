#ifndef DAS_SERVER_LOG_MANAGER_H
#define DAS_SERVER_LOG_MANAGER_H

#include <future>

#include "log_saver/log_saver_controller.h"

namespace Das {
namespace Log_Saver {

class Manager : public Controller
{
    static Manager* _instance;
public:
    static Manager* instance();

    Manager();
    ~Manager();

    void fill_log_value_layers();
    void organize_log_partition();

    void set_devitem_values(QVector<Device_Item_Value> &&data, uint32_t scheme_id);
    QVector<Device_Item_Value> get_devitem_values(uint32_t scheme_id);

    void set_statuses(QVector<DIG_Status>&& data, uint32_t scheme_id);
    std::set<DIG_Status> get_statuses(uint32_t scheme_id);

private:
    void start_long_term_operation(const QString& name, void(Manager::*func)());
    void fill_log_value_layers_impl();
    void organize_log_partition_impl();

    QString _long_term_operation_name;
    future<void> _long_term_operation;
};

Manager* instance();

} // namespace Log_Saver
} // namespace Das

#endif // DAS_SERVER_LOG_MANAGER_H
