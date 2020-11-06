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

    Layers_Filler::start_filling_time().load();
    set_after_insert_log_callback<Log_Value_Item>([](const vector<Log_Value_Item>& data)
    {
        map<uint32_t, qint64> scheme_time;
        map<uint32_t, qint64>::iterator it = scheme_time.end();
        for (const Log_Value_Item& item: data)
        {
            if (it == scheme_time.end() || it->first != item.scheme_id())
            {
                it = scheme_time.find(item.scheme_id());
                if (it == scheme_time.end())
                    it = scheme_time.emplace(item.scheme_id(), numeric_limits<qint64>::max()).first;
            }

            if (it->second > item.timestamp_msecs())
                it->second = item.timestamp_msecs();
        }
        Layers_Filler::start_filling_time().set_scheme_time(scheme_time);
    });
}

Manager::~Manager()
{
    if (_long_term_operation.valid())
        _long_term_operation.get();
    Layers_Filler::start_filling_time().save();

    _instance = nullptr;
}

void Manager::fill_log_value_layers()
{
    start_long_term_operation("fill_log_value_layers", &Manager::fill_log_value_layers_impl);
}

void Manager::organize_log_partition()
{
    start_long_term_operation("organize_log_partition", &Manager::organize_log_partition_impl);
}

void Manager::set_devitem_values(QVector<Device_Item_Value> &&data, uint32_t scheme_id)
{
    for (Device_Item_Value& item: data)
        item.set_scheme_id(scheme_id);
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

    for (DIG_Status& item: data)
        item.set_scheme_id(scheme_id);
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

void Manager::start_long_term_operation(const QString &name, void (Manager::*func)())
{
    auto status = _long_term_operation.valid() ?
                _long_term_operation.wait_for(0s) : future_status::ready;
    if (status == future_status::ready)
    {
        _long_term_operation_name = name;
        _long_term_operation = async(launch::async, [this, name, func]()
        {
            qInfo() << "Begin" << name << "operation";
            auto now = clock::now();
            (this->*func)();

            time_t t = chrono::duration_cast<chrono::seconds>(clock::now() - now).count();
            char data[256];
            strftime(data, 256, "%H:%M:%S", localtime(&t));
            qInfo() << "Operation" << name << "finished. It's take" << data;
        });
    }
    else
        qWarning() << name << "can't be started because" << _long_term_operation_name
                   << "long-term operation is already running.";
}

void Manager::fill_log_value_layers_impl()
{
    Layers_Filler{}();
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
ALTER TABLE das_log_value%1
PARTITION BY RANGE (timestamp_msecs) (
PARTITION `p_year` VALUES LESS THAN (%2),
PARTITION `p_half_year` VALUES LESS THAN (%3),
PARTITION `p_1_month` VALUES LESS THAN (%4),
PARTITION `p_last` VALUES LESS THAN MAXVALUE);
)sql")
            ;

    Base& db = Base::get_thread_local_instance();
    db.exec(sql.arg(QString())
            .arg(ts(1 * 365.25))
            .arg(ts(0.5 * 365.25))
            .arg(ts(1 * 30.4167)));
    db.exec(sql.arg("_minute")
            .arg(ts(1 * 365.25))
            .arg(ts(0.5 * 365.25))
            .arg(ts(1 * 30.4167)));
}

Manager *instance()
{
    return Manager::instance();
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das
