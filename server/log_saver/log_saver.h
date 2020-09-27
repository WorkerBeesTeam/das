#ifndef DAS_SERVER_LOG_SAVER_SAVER_H
#define DAS_SERVER_LOG_SAVER_SAVER_H

#include <queue>

#include <Helpz/db_base.h>

#include "log_saver_base.h"
#include "log_saver_cache.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace Helpz::DB;

template<typename T>
class Saver : public Saver_Base
{
public:
    Saver(int pack_size = 100) :
        _pack_size(pack_size),
        _init_time(DB::Log_Base_Item::current_timestamp())
    {
    }
    ~Saver()
    {
    }

    template<template<typename...> class Container>
    void add(uint32_t scheme_id, Container<T>& data)
    {
        for (T& item: data)
        {
            item.set_scheme_id(scheme_id);

            if constexpr (is_same<T, Log_Value_Item>::value)
            {
                if (!item.need_to_save())
                {
                    check_item_time(item, get_save_time());
                    continue;
                }
            }

            _data.push(item);
        }
    }

    template<template<typename...> class Container = QVector, typename K = typename Helper<T>::Value_Type>
    Container<K> get_cache_data(uint32_t scheme_id)
    {
        boost::shared_lock lock(_cache_mutex);
        auto it = _scheme_cache.find(scheme_id);
        if (it == _scheme_cache.cend())
            return {};

        return it->second.template get_data<Container, K>();
    }

    bool empty_data() const override
    {
        return _data.empty();
    }

    bool empty_cache() const override
    {
        boost::shared_lock lock(_cache_mutex);
        for (const auto& it: _scheme_cache)
            if (!it.second.empty())
                return false;
        return true;
    }

    shared_ptr<Data> get_data_pack() override
    {
        if (is_cache_ready_to_save())
            return get_cache_data_pack();

        vector<T> pack;
        for (int i = 0; i < _pack_size && !_data.empty(); ++i)
        {
            pack.push_back(move(_data.front()));
            _data.pop();
        }
        return make_shared<Data_Impl<T>>(Data::Log, move(pack));
    }

    void process_data_pack(shared_ptr<Data> data) override
    {
        auto pack = static_pointer_cast<Data_Impl<T>>(data);
        if (pack)
        {
            if (data->type() == Data::Log)
                process_data_pack(pack->_data);
            else
                process_cache(pack->_data);
        }
    }

    void erase_empty_cache() override
    {
        lock_guard lock(_cache_mutex);
        for (auto it = _scheme_cache.begin(); it != _scheme_cache.end();)
        {
            if (it->second.empty())
                it = _scheme_cache.erase(it);
            else
                ++it;
        }
    }
private:
    bool is_cache_ready_to_save() const
    {
        return _oldest_cache_time.load() <= clock::now();
    }

    shared_ptr<Data> get_cache_data_pack()
    {
        time_point scheme_oldest_cache_time,
                oldest_cache_time = time_point::max();
        auto ready_to_save_time = clock::now() + 1s;
        vector<T> pack;

        boost::shared_lock lock(_cache_mutex);
        for (auto& it: _scheme_cache)
        {
            scheme_oldest_cache_time = it.second.get_ready_to_save_data(pack, ready_to_save_time);
            if (oldest_cache_time > scheme_oldest_cache_time)
                oldest_cache_time = scheme_oldest_cache_time;
        }

        if (oldest_cache_time < time_point::max())
            _oldest_cache_time = oldest_cache_time;
        return make_shared<Data_Impl<T>>(Data::Values, move(pack));
    }

    void process_data_pack(vector<T>& pack)
    {
        const time_point save_time_point = get_save_time();

        QVariantList values_pack;
        for (auto it = pack.begin(); it != pack.end();)
        {
            if (!Helper<T>::is_comparable() || check_item_time(*it, save_time_point))
            {
                for (size_t i = 1; i < T::COL_COUNT; ++i) // without id
                    values_pack.push_back(T::value_getter(*it, i));

                it = pack.erase(it);
            }
            else
                ++it; // if is some old value, leave it to check duplicate
        }

        Base& db = Base::get_thread_local_instance();

        auto table = db_table<T>();

        if (!pack.empty()) // check duplicates in db
        {
//            SELECT timestamp_msecs, scheme_id, item_id FROM das_log_value WHERE
//            (timestamp_msecs = 1599881793207 AND scheme_id = 5 AND item_id = 2) OR
//            (timestamp_msecs = 1599881765689 AND scheme_id = 8 AND item_id = 61)

            const vector<uint32_t> compared_fields = Helper<T>::get_compared_fields();

            if (!compared_fields.empty())
            {
                QVariantList values;
                QString where = get_compare_where(pack, table, compared_fields, values);
                auto q = db.select(table, where, values, compared_fields);

                while (q.next())
                {
                    for (auto it = pack.begin(); it != pack.end();)
                    {
                        for (size_t i = 0; i < compared_fields.size(); ++i)
                        {
                            if (T::value_getter(*it, compared_fields.at(i)) != q.value(i))
                            {
                                ++it;
                                goto not_equals;
                            }
                        }
                        it = pack.erase(it);
    //                    break; // Can be more than one same duplicate in pack?
                        not_equals:;
                    }
                }
            }

            // add to values_pack if isn't duplicate
            for (const T& item: pack)
                for (size_t i = 1; i < T::COL_COUNT; ++i) // without id
                    values_pack.push_back(T::value_getter(item, i));
        }

        table.field_names().removeFirst(); // remove id
        const QString sql = get_custom_q_array(table, values_pack.size() / table.field_names().size());
        if (!db.exec(sql, values_pack).isActive())
        {
            for (int i = 0; i < values_pack.size(); )
            {
                auto dbg = qCritical().noquote() << "Lost log " << typeid(T).name();
                for (const QString& name: table.field_names())
                {
                    dbg << name << '=' << values_pack.at(i).toString() << ' ';
                    ++i;
                }
            }
            // TODO: Try to insert each separately. With check duplicate?
        }
    }

    void process_cache(vector<T>& pack)
    {
        // UPDATE dig_param_value SET timestamp_msecs=? AND user_id=? AND value=?
        // WHERE scheme_id=? AND group_param_id=? AND timestamp_msecs<=?

        Base& db = Base::get_thread_local_instance();

        using V = typename Helper<T>::Value_Type;
        Table table = db_table<V>();
        const vector<uint32_t> updating_fields = Helper<T>::get_values_fields();
        const vector<uint32_t> compared_fields = Helper<T>::get_compared_fields();
        const QString where = get_update_where(table, compared_fields);
        const QString sql = db.update_query(table, updating_fields.size() + compared_fields.size(), where, updating_fields);

        QVariantList values;

        for(const T& item: pack)
        {
            values.clear();
            for (uint32_t field: updating_fields)
                values.push_back(T::value_getter(item, field));
            for (uint32_t field: compared_fields)
                values.push_back(T::value_getter(item, field));

            const QSqlQuery q = db.exec(sql, values);
            if (!q.isActive() || q.numRowsAffected() <= 0)
            {
                // TODO: SELECT before insert for reasons when CLIENT_FOUND_ROWS=1 opt is not set.
                if (!db.insert(table, V::to_variantlist(item)))
                {
                    // TODO: do something
                }
            }
        }
    }

    QString get_update_where(const Table& table, const vector<uint32_t>& compared_fields) const
    {
        QString where;
        for (uint32_t field: compared_fields)
        {
            if (!where.isEmpty())
                where += " AND ";
            where += table.field_names().at(field);
            if (field == T::COL_timestamp_msecs)
                where += '<'; // = for timestamp_msecs = 0, < for others
            where += "=?";
        }
        return where;
    }

    time_point get_save_time() const
    {
        return clock::now() + 15s;
    }

    bool check_item_time(const T& item, const time_point save_time_point)
    {
        boost::upgrade_lock shared_lock(_cache_mutex);
        Cache_Data<T>& cache = get_cache(item.scheme_id(), shared_lock);

        cache.add(item, save_time_point);
        if (_oldest_cache_time.load().time_since_epoch() == clock::duration::zero())
            _oldest_cache_time = save_time_point;

        if (cache._last_time < item.timestamp_msecs())
        {
            cache._last_time = item.timestamp_msecs();
            return true;
        }
        return false;
    }

    Cache_Data<T>& get_cache(uint32_t scheme_id, boost::upgrade_lock<boost::shared_mutex>& shared_lock)
    {
        auto it = _scheme_cache.find(scheme_id);
        if (it == _scheme_cache.end())
        {
            boost::upgrade_to_unique_lock lock(shared_lock);
            it = _scheme_cache.find(scheme_id);
            if (it == _scheme_cache.end())
                it = _scheme_cache.emplace(scheme_id, _init_time).first;
        }
        return it->second;
    }

    QString get_custom_q_array(const Table& table, int row_count) const
    {
        // " + (force ? QString("IGNORE ") : QString()) + "
        return "INSERT INTO " +
                table.name() + '(' + table.field_names().join(',') + ") VALUES" +
                Base::get_q_array(table.field_names().size(), row_count);
    }

    QString get_compare_where(const vector<T>& pack, const Table& table,
                              const vector<uint32_t>& compared_fields, QVariantList& values) const
    {
        QString item_template = get_where_item_template(table, compared_fields);
        QString where;

        for (const T& item: pack)
        {
            if (where.isEmpty())
                where = "WHERE (";
            else
                where += ") OR (";
            where += item_template;

            for (uint32_t field_index: compared_fields)
                values.push_back(T::value_getter(item, field_index));
        }
        where += ')';
        return where;
    }

    QString get_where_item_template(const Table& table, const vector<uint32_t>& compared_fields) const
    {
        QString item_template;
        for (uint32_t field_index: compared_fields)
        {
            if (!item_template.isEmpty())
                item_template += " AND ";
            item_template += table.field_names().at(field_index);
            item_template += "=?";
        }
        return item_template;
    }

    const int _pack_size;

    queue<T> _data;

    const qint64 _init_time;
    mutable boost::shared_mutex _cache_mutex;
    map<uint32_t/*scheme_id*/, Cache_Data<T>> _scheme_cache;
    atomic<time_point> _oldest_cache_time;
};

template<>
inline void Saver<Log_Event_Item>::process_cache(vector<Log_Event_Item>&) { assert(false && "No cache for Log_Event_Item"); }

template<>
inline void Saver<Log_Status_Item>::process_cache(vector<Log_Status_Item>& pack)
{
    // DELETE FROM dig_status WHERE (scheme_id=? AND group_id=? AND status_id=?) OR ...
    // INSERT INTO dig_status (id, timestamp_msecs, user_id, group_id, status_id, args, scheme_id) VALUES(?,?,?,?,?,?,?),...

    using T = Log_Status_Item;
    using V = Helper<T>::Value_Type; // DIG_Status

    QString sql;
    QVariantList values;
    Table table = db_table<V>();
    Base& db = Base::get_thread_local_instance();

    vector<Log_Status_Item> delete_pack;
    for (auto it = pack.begin(); it != pack.end();)
    {
        if (it->is_removed())
        {
            delete_pack.push_back(move(*it));
            it = pack.erase(it);
        }
        else
            ++it;
    }

    if (!delete_pack.empty())
    {
        vector<uint32_t> compared_fields = Helper<T>::get_compared_fields();
        compared_fields.erase(remove(compared_fields.begin(), compared_fields.end(), T::COL_timestamp_msecs), compared_fields.end());

        sql = "DELETE FROM ";
        sql += table.name();
        sql += ' ';
        sql += get_compare_where(delete_pack, db_table<T>(), compared_fields, values);

        if (!db.exec(sql, values).isActive())
        {
            // TODO: Do something
        }

        values.clear();
    }

    if (!pack.empty())
    {
        for (const T& item: pack)
            values += V::to_variantlist(item);

        sql = get_custom_q_array(table, pack.size());
        if (!db.exec(sql, values).isActive())
        {
            // TODO: Do something
        }
    }
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_SAVER_H
