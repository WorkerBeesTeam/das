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
    if (_fill_log_value_layers_thread.joinable())
        _fill_log_value_layers_thread.join();

    _instance = nullptr;
}

void Manager::fill_log_value_layers()
{
    if (_fill_log_value_layers_thread.joinable())
        qWarning() << "Function fill_log_value_layers is already runing";
    else
        _fill_log_value_layers_thread = thread(&Manager::fill_log_value_layers_impl, this);
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

void Manager::fill_log_value_layers_impl()
{
    const int minute = 1 * 60 * 1000;
    const int hour = 1 * 60 * minute;
    const int day = 1 * 24 * hour;

    fill_log_value_layer("minute", minute);
    fill_log_value_layer("hour", hour);
    fill_log_value_layer("day", day);
}

void Manager::fill_log_value_layer(const QString &name, int time_count)
{
    auto get_prev_time = [](qint64 ts, int time_count)
    {
        return ts - (ts % time_count);
    };
    auto get_next_time = [](qint64 ts, int time_count)
    {
        return ts + (time_count - (ts % time_count));
    };

    qint64 start_ts, end_ts,
            now_ts = chrono::duration_cast<chrono::milliseconds>((clock::now() - 5min).time_since_epoch()).count();
    now_ts = get_prev_time(now_ts, time_count);

    Table table = db_table<DB::Log_Value_Item>();
    table.set_name(table.name() + '_' + name);

    Base& db = Base::get_thread_local_instance();
    auto q = db.select(table, "ORDER BY timestamp_msecs DESC LIMIT 1", {},
              {DB::Log_Value_Item::COL_timestamp_msecs});
    if (q.next())
        start_ts = get_next_time(q.value(0).toLongLong(), time_count);
    else
    {
        q = db.select(db_table<DB::Log_Value_Item>(), "ORDER BY timestamp_msecs ASC LIMIT 1", {},
                  {DB::Log_Value_Item::COL_timestamp_msecs});
        if (!q.next())
        {
            qWarning() << "das_log_value is empty?";
            return; // TODO: ?
        }
        start_ts = get_prev_time(q.value(0).toLongLong(), time_count);
    }

    end_ts = start_ts + time_count;

    const QString sql = R"sql(
INSERT INTO %1 (timestamp_msecs, user_id, item_id, scheme_id, value, raw_value)
SELECT timestamp_msecs as ts, user_id, item_id, scheme_id,
IF (value REGEXP '^-?[0-9]+(\.[0-9]+)?$', ((MAX(value+0) + MIN(value+0) +
substring_index(group_concat(value order by timestamp_msecs), ',', 1 ) +
substring_index(group_concat(value order by timestamp_msecs), ',', -1 )) / 4), value) as v,
((MAX(raw_value+0) + MIN(raw_value+0) +
substring_index(group_concat(raw_value order by timestamp_msecs), ',', 1 ) +
substring_index(group_concat(raw_value order by timestamp_msecs), ',', -1 )) / 4) as rv
FROM das_log_value
WHERE timestamp_msecs >= ? AND timestamp_msecs < ?
GROUP BY item_id, scheme_id
)sql";

    while (end_ts <= now_ts)
    {
        q = db.exec(sql.arg(table.name()), {start_ts, end_ts});
        auto dbg = qDebug() << "active" << q.isActive();
        dbg << "next" << q.next();

        start_ts = end_ts;
        end_ts += time_count;
    }

    /* Предлагается выбирать из главной таблицы 4 значения за промежуток (например за минуту),
     * минимальное, максимальное, первое и последнее.
     *
pl: BEGIN
DECLARE tbl_name, old_tz VARCHAR(32);
DECLARE start_ts, end_ts, now_ts BIGINT;

SET old_tz = @@session.time_zone;
SET @@session.time_zone='+00:00';
SET now_ts = ROUND(UNIX_TIMESTAMP(CURTIME(4)) * 1000) - (min_for_now * 60 * 1000);
SET now_ts = get_prev_time(now_ts, time_count);

SET @@session.time_zone=old_tz;

SET tbl_name = CONCAT("das_log_value", name);
PREPARE stmt FROM CONCAT("SELECT get_next_time(timestamp_msecs, ", time_count, ") INTO @start_ts FROM ", tbl_name, " ORDER BY timestamp_msecs DESC LIMIT 1");
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

IF @start_ts IS NOT NULL THEN
    SET start_ts = @start_ts;
ELSE
    SELECT get_next_time(timestamp_msecs, time_count INTO start_ts FROM das_log_value ORDER BY timestamp_msecs DESC LIMIT 1;
END IF;

IF start_ts IS NULL THEN
    LEAVE pl;
END IF;

SET end_ts = start_ts + time_count;

SELECT start_ts, now_ts, end_ts;

END

*/
}

Manager *instance()
{
    return Manager::instance();
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das
