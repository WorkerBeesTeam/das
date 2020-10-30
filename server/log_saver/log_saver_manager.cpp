#include <future>

#include <Helpz/db_builder.h>

#include "log_saver_layers_filler.h"
#include "log_saver_manager.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace Helpz::DB;

/*static*/ Manager* Manager::_instance = nullptr;
/*static*/ Manager* Manager::instance()
{
    return _instance;
}

Manager::Manager() :
    Controller{}
{
    _instance = this;

    Layers_Filler::load_start_filling_time();
    set_after_insert_log_callback<Log_Value_Item>([](const vector<Log_Value_Item>& data)
    {
        qint64 min_ts = numeric_limits<qint64>::max();
        for (const Log_Value_Item& item: data)
            if (min_ts > item.timestamp_msecs())
                min_ts = item.timestamp_msecs();
        Layers_Filler::set_start_filling_time(min_ts);
    });
}

Manager::~Manager()
{
    if (_long_term_operation_thread.joinable())
        _long_term_operation_thread.join();
    Layers_Filler::save_start_filling_time();

    _instance = nullptr;
}

void Manager::fill_log_value_layers()
{
    start_log_term_operation("fill_log_value_layers", &Manager::fill_log_value_layers_impl);
}

void Manager::organize_log_partition()
{
    start_log_term_operation("organize_log_partition", &Manager::organize_log_partition_impl);
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
    QVector<Device_Item_Value> device_item_values = db_build_list<Device_Item_Value>(db, "WHERE scheme_id=?", {scheme_id});
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
    QVector<DIG_Status> statuses = db_build_list<DIG_Status>(db, "WHERE scheme_id=?", {scheme_id});
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
    vector<DIG_Status> db_statuses = db_build_list<DIG_Status, vector>(db, "WHERE scheme_id=?", {scheme_id});
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

void Manager::start_log_term_operation(const QString &name, void (Manager::*func)())
{
    if (_long_term_operation_thread.joinable())
        qWarning() << name << "can't be started because" << _long_term_operation_name
                   << "long-term operation is already running.";
    else
    {
        _long_term_operation_name = name;
        _long_term_operation_thread = thread(func, this);
    }
}

void Manager::fill_log_value_layers_impl()
{
    Layers_Filler{}();
    qInfo() << "Fill log_value layers finished";
}

void Manager::organize_log_partition_impl()
{
    qint64 now_ts = DB::Log_Base_Item::current_timestamp();
    auto ts = [now_ts](qint64 time_count_day)
    {
        time_count_day *= 1 * 24 * 60 * 60 * 1000;
        return Layers_Filler::get_prev_time(now_ts - time_count_day / 2, time_count_day);
    };

    const QString sql = QString(R"sql(
ALTER TABLE das_log_value%%1
PARTITION BY RANGE (timestamp_msecs) (
PARTITION `p_year` VALUES LESS THAN (%1),
PARTITION `p_half_year` VALUES LESS THAN (%2),
PARTITION `p_1_month` VALUES LESS THAN (%3),
PARTITION `p_last` VALUES LESS THAN MAXVALUE);
)sql")
            .arg(ts(1 * 365.25))
            .arg(ts(0.5 * 365.25))
            .arg(ts(1 * 30.4167));

    Base& db = Base::get_thread_local_instance();
    db.exec(sql.arg(QString()));
    db.exec(sql.arg("_minute"));
    qInfo() << "Organize log_value partitions finished";
}

Manager *instance()
{
    return Manager::instance();
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das
