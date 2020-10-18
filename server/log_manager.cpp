#include "log_manager.h"

namespace Das {
namespace Server {

using namespace std;

/*static*/ Log_Manager* Log_Manager::_instance = nullptr;
/*static*/ Log_Manager *Log_Manager::instance()
{
    return _instance;
}

Log_Manager::Log_Manager()
{
    _instance = this;
}

Log_Manager::~Log_Manager()
{
    _instance = nullptr;
}

void Log_Manager::stop()
{

}

Log_Saver::Controller &Log_Manager::ctrl()
{
    return _ctrl;
}

void Log_Manager::set_devitem_values(QVector<Log_Value_Item> &&data, uint32_t scheme_id)
{
    _ctrl.set_cache_data(move(data), scheme_id);
}

QVector<Device_Item_Value> Log_Manager::get_devitem_values(uint32_t scheme_id)
{
    // Достать из базы текущие значения и Составить актуальный массив с данными
    return _ctrl.get_cache_data<Log_Value_Item>(scheme_id);
}

void Log_Manager::set_statuses(QVector<Log_Status_Item> &&data, uint32_t scheme_id)
{
    // Достать из базы текущие состояние и Составить актуальный массив с данными
    _ctrl.set_cache_data(move(data), scheme_id);
}

set<DIG_Status> Log_Manager::get_statuses(uint32_t scheme_id)
{
    // Достать из базы текущие значения и Составить актуальный массив с данными
    return _ctrl.get_cache_data<Log_Status_Item, set, DIG_Status>(scheme_id);
}

} // namespace Server
} // namespace Das
