#include <future>

#include <Helpz/db_builder.h>

#include "log_saver_manager.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace std;
using namespace Helpz::DB;

/*static*/ Manager* Manager::_instance = nullptr;
/*static*/ Manager *Manager::instance()
{
    return _instance;
}

Manager::Manager(size_t thread_count, size_t max_pack_size, chrono::seconds time_in_cache) :
    Controller{thread_count, max_pack_size, time_in_cache}
{
    _instance = this;
}

Manager::~Manager()
{
    _instance = nullptr;
}

void Manager::set_devitem_values(QVector<Device_Item_Value> &&data, uint32_t scheme_id)
{
    set_cache_data<Log_Value_Item>(move(data), scheme_id);
}

QVector<Device_Item_Value> Manager::get_devitem_values(uint32_t scheme_id)
{
    // Достать из базы текущие значения и Составить актуальный массив с данными
    future<QVector<Device_Item_Value>> get_devitem_task = async([this, scheme_id]()
    {
        return get_cache_data<Log_Value_Item>(scheme_id);
    });

    Base& db = Base::get_thread_local_instance();
    QVector<Device_Item_Value> device_item_values = db_build_list<Device_Item_Value>(db, "scheme_id=?", {scheme_id});
    QVector<Device_Item_Value> cached_values = get_devitem_task.get();
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

void Manager::set_statuses(QVector<DIG_Status> &&data, uint32_t scheme_id)
{
    // В базе могут быть состояния которых уже нет
    // нужно дополнить массив новых состояний теми которые нужно удалить из базы

    auto find_status = [&data](const DIG_Status& status)
    {
        for (const DIG_Status& item: data)
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
            data.push_back(move(status));
        }

    set_cache_data<Log_Status_Item>(move(data), scheme_id);
}

set<DIG_Status> Manager::get_statuses(uint32_t scheme_id)
{
    future<set<DIG_Status>> get_cache_data_task = async([this, scheme_id]()
    {
        return get_cache_data<Log_Status_Item, set>(scheme_id);
    });

    Base& db = Base::get_thread_local_instance();
    vector<DIG_Status> db_statuses = db_build_list<DIG_Status, vector>(db, "scheme_id=?", {scheme_id});
    vector<DIG_Status>::iterator db_it;
    set<DIG_Status> statuses = get_cache_data_task.get();
    for (auto it = statuses.begin(); it != statuses.end();)
    {
        db_it = find_if(db_statuses.begin(), db_statuses.end(), [&it](const DIG_Status& t1) -> bool{
            return Cache_Helper<DIG_Status>::is_item_equal(t1, *it);
        });
        if (db_it != db_statuses.end())
            db_statuses.erase(db_it);

        if (it->direction() == DIG_Status::SD_DEL)
            it = statuses.erase(it);
        else
            ++it;
    }
    for (DIG_Status& item: db_statuses)
        statuses.insert(move(item));
    return statuses;
}

Manager *instance()
{
    return Manager::instance();
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das
