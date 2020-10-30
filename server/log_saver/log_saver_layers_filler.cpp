
#include <QSqlError>

#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include "log_saver_config.h"
#include "log_saver_layers_filler.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace Helpz::DB;

struct Item_Status
{
    uint32_t _x;
    uint64_t _ts;

    map<uint32_t, uint32_t> _users;
    map<uint32_t, uint32_t>::iterator _user_it;
    DB::Log_Value_Item _item;

    void reset(uint32_t item_id)
    {
        _ts = 0;
        _users.clear();
        _value.reset();
        _raw_value.reset();
        _item.set_user_id(0);
        _item.set_item_id(item_id);
    }

    void analyze(const DB::Log_Value_Item& item)
    {
        _ts += item.timestamp_msecs();
        if (item.user_id())
        {
            _user_it = _users.find(item.user_id());
            if (_user_it == _users.end())
                _users.emplace(item.user_id(), 1);
            else
                ++_user_it->second;
        }

        _value.analyze(item.value());
        _raw_value.analyze(item.raw_value());
    }

    void finalize(size_t data_count)
    {
        _x = 0;
        for (auto&& it: _users)
        {
            if (_x < it.second)
            {
                _x = it.second;
                _item.set_user_id(it.first);
            }
        }

        _item.set_timestamp_msecs(_ts / data_count);
        _item.set_value(_value.finalize(data_count / 2.));
        _item.set_raw_value(_raw_value.finalize(data_count / 2.));
    }

    struct Value_Status
    {
        uint32_t _null_count, _value_count;
        long double _value;
        map<QString, uint32_t> _text_count;
        map<QString, uint32_t>::iterator _text_it;
        QString _text;

        void reset()
        {
            _null_count = 0;
            _value_count = 0;
            _value = 0;
            _text_count.clear();
            _text.clear();
        }

        QVariant finalize(double half_data_count)
        {
            if (_null_count > half_data_count)
                return {};
            else if (_value_count > half_data_count)
                return static_cast<double>(_value / _value_count);

            _value_count = 0;
            for (auto&& it: _text_count)
            {
                if (_value_count < it.second)
                {
                    _value_count = it.second;
                    _text = move(it.first);
                }
            }
            return _text.isEmpty() ? QVariant{} : _text;
        }

        void analyze(const QVariant& value)
        {
            if (value.isValid())
            {
                if (value.type() == QVariant::String
                    || value.type() == QVariant::ByteArray)
                {
                    _text = value.toString();
                    if (_text.size() > static_cast<int>(Log_Saver::Config::get()._layer_text_max_count))
                        _text.resize(Log_Saver::Config::get()._layer_text_max_count);
                    _text_it = _text_count.find(_text);
                    if (_text_it == _text_count.end())
                        _text_count.emplace(_text, 1);
                    else
                        ++_text_it->second;
                }
                else
                {
                    _value += value.toDouble();
                    ++_value_count;
                }
            }
            else
                ++_null_count;
        }
    } _value, _raw_value;
};

// ---------------------------------------------

/*static*/ atomic<qint64> Layers_Filler::_start_filling_time{0};

/*static*/ void Layers_Filler::set_start_filling_time(qint64 ts)
{
    qint64 old_ts = _start_filling_time.load();
    if (old_ts == 0 || old_ts > ts)
    {
        if (!_start_filling_time.compare_exchange_strong(old_ts, ts))
            set_start_filling_time(ts);
    }
    else if (old_ts != _start_filling_time.load())
        set_start_filling_time(ts);
}

/*static*/ void Layers_Filler::load_start_filling_time()
{
    Base& db = Base::get_thread_local_instance();
    auto q = db.select({"das_settings", {}, {"value"}}, "WHERE param = 'log_value_layer_time'");
    if (q.next())
        _start_filling_time = q.value(0).toLongLong();
}

/*static*/ void Layers_Filler::save_start_filling_time()
{
    const QString sql = "INSERT INTO das_settings(param, value) VALUES(?,?) ON DUPLICATE KEY UPDATE value = VALUES(value)";
    Base& db = Base::get_thread_local_instance();
    db.exec(sql, {"log_value_layer_time", _start_filling_time.load()});
}

/*static*/ qint64 Layers_Filler::get_prev_time(qint64 ts, qint64 time_count) const
{
    return ts - (ts % time_count);
}

/*static*/ qint64 Layers_Filler::get_next_time(qint64 ts, qint64 time_count) const
{
    return ts + (time_count - (ts % time_count));
}

Layers_Filler::Layers_Filler() :
    _log_value_table(db_table<DB::Log_Value_Item>()),
    _layer_table(_log_value_table),
    _layers_info{
        {1 * 60 * 1000, "minute"},
        {1 * 60 * 60 * 1000, "hour"},
        {1 * 24 * 60 * 60 * 1000, "day"}
    }
{
}

void Layers_Filler::operator ()()
{
    _last_end_time = _start_filling_time.load();

    qint64 final_ts, max_ts = 0;

    for (auto&& it: _layers_info)
    {
        final_ts = fill_layer(it.first, it.second);
        if (max_ts < final_ts)
            max_ts = final_ts;

        _log_value_table.set_name(DB::Log_Value_Item::table_name() + '_' + it.second);
    }

    set_start_filling_time(max_ts);
}

qint64 Layers_Filler::fill_layer(qint64 time_count, const QString &name)
{
    _layer_table.set_name(_log_value_table.name() + '_' + name);

    qint64 start_ts = get_start_timestamp(time_count);
    if (start_ts == 0)
    {
        qWarning() << "das_log_value is empty?";
        return 0; // TODO: ?
    }

    Base& db = Base::get_thread_local_instance();

    if (_last_end_time != 0 && start_ts > _last_end_time)
    {
        start_ts = get_prev_time(_last_end_time, time_count);
        db.del(_layer_table.name(), "timestamp_msecs >= ?", {start_ts});
    }

    qint64 end_ts = start_ts + time_count,
           final_ts = get_final_timestamp(time_count);

    QSqlQuery insert_query{db.database()}; // DB open in get_start_timestamp in Base::select
    insert_query.prepare(db.insert_query(_layer_table, _layer_table.field_names().size()));
    QVariantList values;

    QSqlQuery log_query{db.database()};
    log_query.prepare(db.select_query(_log_value_table, "WHERE timestamp_msecs >= ? AND timestamp_msecs < ?"));

    Data_Type data;

    while (end_ts <= final_ts)
    {
        log_query.addBindValue(start_ts);
        log_query.addBindValue(end_ts);
        if (!log_query.exec())
        {
            qCritical() << "Layer filler: Can't select data from das_log_value:" << log_query.lastError().text();
            return start_ts;
        }

        while (log_query.next())
        {
            DB::Log_Value_Item item = db_build<DB::Log_Value_Item>(log_query);
            data[item.scheme_id()][item.item_id()].push_back(move(item));
        }

        if (!data.empty())
        {
            vector<DB::Log_Value_Item> average_data = get_average_data(data);
            for (const DB::Log_Value_Item& item: average_data)
            {
                values = DB::Log_Value_Item::to_variantlist(item);
                for (const QVariant& value: values)
                    insert_query.addBindValue(value);
                if (!insert_query.exec())
                {
                    qWarning() << "Can't insert average log value for layer:" << name
                               << "error:" << insert_query.lastError().text()
                               << "data:" << values;
                }
            }

            data.clear();
        }

        start_ts = end_ts;
        end_ts += time_count;
    }

    return final_ts;

    // TODO: Сделать Das_Settings::scheme_id возможным NULL.

    // TODO: Сделать запрос всей структуры схемы из фронта через webapi (api/v2)
    // и в webapi получать кэшированные значения у сервера перед отдачей структуры во фронт.
    // DevItemValue и Status уже получаются, нужны Mode и Param

    // TODO: При отдаче логов с сервера последние 10 минут беруться из основной таблицы.
    // Нужно эти данные усреднять в зависимости от масштаба.

    // TODO: При удалении device_item или scheme нужно также очищать таблицы с логами.

    // TODO: Хранить файлы (картинки например) на диске, а в базе только путь к ним.
}

qint64 Layers_Filler::get_start_timestamp(qint64 time_count) const
{
    Base& db = Base::get_thread_local_instance();
    auto q = db.select(_layer_table, "ORDER BY timestamp_msecs DESC LIMIT 1", {},
              {DB::Log_Value_Item::COL_timestamp_msecs});
    if (q.next())
        return get_next_time(q.value(0).toLongLong(), time_count);
    else
    {
        q = db.select(_log_value_table, "ORDER BY timestamp_msecs ASC LIMIT 1", {},
                  {DB::Log_Value_Item::COL_timestamp_msecs});
        if (q.next())
            return get_prev_time(q.value(0).toLongLong(), time_count);;
    }
    return 0;
}

qint64 Layers_Filler::get_final_timestamp(qint64 time_count) const
{
    chrono::minutes minus_min{Config::get()._layer_min_for_now_minute};
    qint64 minus_ms = chrono::duration_cast<chrono::milliseconds>(minus_min).count();
    return get_prev_time(DB::Log_Base_Item::current_timestamp() - minus_ms, time_count);
}

vector<DB::Log_Value_Item> Layers_Filler::get_average_data(const Layers_Filler::Data_Type &data)
{
    vector<DB::Log_Value_Item> average;

    // Собирем отдельно текст и NULL. Если кол-во текста или NULL
    // больше чем чисел, то берём текст который чаще всего встречался или NULL
    Item_Status s;

    for (auto&& scheme_data: data)
    {
        s._item.set_scheme_id(scheme_data.first);
        for (auto&& devitem_data: scheme_data.second)
        {
            s.reset(devitem_data.first);

            for (const DB::Log_Value_Item& item: devitem_data.second)
                s.analyze(item);

            s.finalize(devitem_data.second.size());
            average.push_back(s._item);
        }
    }

    sort(average.begin(), average.end(), [](const DB::Log_Value_Item& it1, const DB::Log_Value_Item& it2)
    {
        return it1.timestamp_msecs() < it2.timestamp_msecs();
    });

    return average;
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das
