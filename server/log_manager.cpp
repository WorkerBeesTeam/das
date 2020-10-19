#include <Helpz/db_builder.h>

#include "log_manager.h"

namespace Das {
namespace Server {

using namespace std;
using namespace Helpz::DB;

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
    Base& db = Base::get_thread_local_instance();
    QVector<Device_Item_Value> device_item_values = db_build_list<Device_Item_Value>(db, "scheme_id=?", {scheme_id});
    QVector<Device_Item_Value> cached_values = _ctrl.get_cache_data<Log_Value_Item>(scheme_id);
    for (const Device_Item_Value& value: cached_values)
    {
        for (Device_Item_Value& orig: cached_values)
            if (orig.item_id() == value.item_id())
            {
                orig = value;
                break;
            }
    }
    return device_item_values;
}

void Log_Manager::set_statuses(QVector<Log_Status_Item> &&data, uint32_t scheme_id)
{
    // В базе скорее всего будут состояния которых уже нет
    // нужно дополнить массив новых состояний теми которые нужно удалить из базы

    auto find_status = [&data](const DIG_Status& status)
    {
        for (const Log_Status_Item& item: data)
            if (item.group_id() == status.group_id()
                && item.status_id() == status.status_id())
                return true;
        return false;
    };

    Base& db = Base::get_thread_local_instance();
    QVector<DIG_Status> statuses = db_build_list<DIG_Status>(db, "scheme_id=?", {scheme_id});
    for (DIG_Status& status: statuses)
        if (!find_status(status))
        {
            status.set_direction(DIG_Status::SD_DEL);
            Log_Status_Item item = reinterpret_cast<Log_Status_Item&&>(move(status));
            data.push_back(move(item));
        }

    _ctrl.set_cache_data(move(data), scheme_id);
}

set<DIG_Status> Log_Manager::get_statuses(uint32_t scheme_id)
{
    set<DIG_Status> statuses = _ctrl.get_cache_data<Log_Status_Item, set, DIG_Status>(scheme_id);
    if (statuses.empty())
    {
        Base& db = Base::get_thread_local_instance();
        statuses = db_build_list<DIG_Status, set>(db, "scheme_id=?", {scheme_id});
    }
    else
    {
        for (auto it = statuses.begin(); it != statuses.end();)
        {
            if (it->direction() == DIG_Status::SD_DEL)
                it = statuses.erase(it);
            else
                ++it;
        }
    }
    return statuses;
}

} // namespace Server
} // namespace Das
