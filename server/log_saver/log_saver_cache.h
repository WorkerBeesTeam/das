#ifndef DAS_SERVER_LOG_SAVER_CACHE_H
#define DAS_SERVER_LOG_SAVER_CACHE_H

#include <Helpz/db_base.h>

#include "log_saver_base.h"
#include "log_saver_cache_data.h"

namespace Das {
namespace Server {
namespace Log_Saver {

template<typename T>
class Saver_Cache
{
public:
    void set_data(QVector<T>&& data, uint32_t scheme_id, chrono::seconds time_in_cache)
    {
        boost::shared_lock lock(_mutex);
        Cache_Data<T>& cache = get_cache(scheme_id, lock);
        cache.set_data(move(data), clock::now() + time_in_cache);
    }

    template<template<typename...> class Container = QVector>
    Container<T> get_data(uint32_t scheme_id)
    {
        boost::shared_lock lock(_mutex);
        auto it = _scheme_data.find(scheme_id);
        if (it == _scheme_data.cend())
            return {};

        return it->second.template get_data<Container>();
    }

    template<typename Log_Type, template<typename...> class Container>
    void add_data(uint32_t scheme_id, const Container<Log_Type>& data, chrono::seconds time_in_cache)
    {
        boost::shared_lock lock(_mutex);
        Cache_Data<T>& cache = get_cache(scheme_id, lock);

        const time_point save_time_point = clock::now() + time_in_cache;
        cache.add(data, save_time_point);
        if (_oldest_time.load() > save_time_point)
            _oldest_time = save_time_point;
    }

    time_point get_oldest_time() const { return _oldest_time.load(); }
    bool is_ready_to_save(bool force = false) const { return force || _oldest_time.load() <= clock::now(); }

    shared_ptr<Data> get_data_pack(size_t max_pack_size = 100, bool force = false)
    {
        bool is_empty;
        time_point scheme_oldest_time,
                oldest_time = time_point::max();
        auto ready_to_save_time = force ? time_point::max() : clock::now() + 1s;
        vector<T> pack;

        {
            boost::shared_lock lock(_mutex);
            for (auto& it: _scheme_data)
            {
                scheme_oldest_time = it.second.get_ready_to_save_data(pack, ready_to_save_time,
                                                                            max_pack_size, is_empty);
                if (oldest_time > scheme_oldest_time)
                    oldest_time = scheme_oldest_time;
            }

            _oldest_time = oldest_time;
        }
        if (is_empty)
            erase_empty();
        return make_shared<Data_Impl<T>>(Data::Cache, move(pack));
    }

    void process(shared_ptr<Data> data)
    {
        auto t_data = static_pointer_cast<Data_Impl<T>>(data);
        process(t_data->_data);
    }

    bool empty() const
    {
        boost::shared_lock lock(_mutex);
        for (const auto& it: _scheme_data)
            if (!it.second.empty())
                return false;
        return true;
    }

private:

    void erase_empty()
    {
        lock_guard lock(_mutex);
        for (auto it = _scheme_data.begin(); it != _scheme_data.end();)
        {
            if (it->second.empty())
                it = _scheme_data.erase(it);
            else
                ++it;
        }
    }

    void process(vector<T>& pack) { process_impl(pack); }
    void process_impl(vector<T>& pack)
    {
        // UPDATE dig_param_value SET timestamp_msecs=? AND user_id=? AND value=?
        // WHERE scheme_id=? AND group_param_id=? AND timestamp_msecs<=?

        Base& db = Base::get_thread_local_instance();

        Table table = db_table<T>();
        const vector<uint32_t> updating_fields = Cache_Helper<T>::get_values_fields();
        const vector<uint32_t> compared_fields = Cache_Helper<T>::get_compared_fields();
        const QString where = get_update_where(table, compared_fields);
        const QString sql = db.update_query(table, updating_fields.size() + compared_fields.size(), where, updating_fields);

        QVariantList values;

        for (const T& item: pack)
        {
            values.clear();
            for (uint32_t field: updating_fields)
                values.push_back(T::value_getter(item, field));
            for (uint32_t field: compared_fields)
                values.push_back(T::value_getter(item, field));

            const QSqlQuery q = db.exec(sql, values);
            if (!q.isActive() || q.numRowsAffected() <= 0)
            {
                vector<uint32_t> sel_cmp_fields = compared_fields;
                sel_cmp_fields.erase(remove(sel_cmp_fields.begin(), sel_cmp_fields.end(), T::COL_timestamp_msecs), sel_cmp_fields.end());
                QString select_sql = "SELECT COUNT(*) FROM " + table.name() + " WHERE " + get_update_where(table, sel_cmp_fields);

                values.clear();
                for (uint32_t field: sel_cmp_fields)
                    values.push_back(T::value_getter(item, field));

                auto q = db.exec(select_sql, values);
                if (q.isActive() && q.next() && q.value(0).toInt() == 0)
                {
                    if (!db.insert(table, T::to_variantlist(item)))
                    {
                        // TODO: do something
                    }
                }
            }
        }
    }

    Cache_Data<T>& get_cache(uint32_t scheme_id, boost::shared_lock<boost::shared_mutex>& shared_lock)
    {
        auto it = _scheme_data.find(scheme_id);
        if (it == _scheme_data.end())
        {
            shared_lock.unlock();
            {
                lock_guard lock(*shared_lock.mutex());
                it = _scheme_data.find(scheme_id);
                if (it == _scheme_data.end())
                    _scheme_data.emplace(scheme_id, Cache_Data<T>{});
            }
            shared_lock.lock();
            return get_cache(scheme_id, shared_lock);
        }
        return it->second;
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

    QString get_compare_where(const vector<T>& pack, const Table& table,
                              const vector<uint32_t>& compared_fields, QVariantList& values, bool add_where_keyword = true) const
    {
        QString item_template = get_where_item_template(table, compared_fields);
        QString where;

        for (const T& item: pack)
        {
            if (where.isEmpty())
            {
                if (add_where_keyword)
                    where = "WHERE ";
                where += '(';
            }
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

    mutable boost::shared_mutex _mutex;
    map<uint32_t/*scheme_id*/, Cache_Data<T>> _scheme_data;
    atomic<time_point> _oldest_time = time_point::max();
};

template<>
inline void Saver_Cache<DIG_Status>::process(vector<DIG_Status>& pack)
{
    // DELETE FROM dig_status WHERE (scheme_id=? AND group_id=? AND status_id=?) OR ...
    // INSERT INTO dig_status (id, timestamp_msecs, user_id, group_id, status_id, args, scheme_id) VALUES(?,?,?,?,?,?,?),...

    vector<DIG_Status> delete_pack;
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
        vector<uint32_t> compared_fields = Cache_Helper<DIG_Status>::get_compared_fields();
        compared_fields.erase(remove(compared_fields.begin(), compared_fields.end(), DIG_Status::COL_timestamp_msecs),
                              compared_fields.end());

        const Table table = db_table<DIG_Status>();
        QVariantList values;
        const QString sql = get_compare_where(delete_pack, table, compared_fields, values, false);

        Base& db = Base::get_thread_local_instance();
        if (!db.del(table.name(), sql, values).isActive())
        {
            // TODO: Do something
        }

        values.clear();
    }

    if (!pack.empty())
        process_impl(pack);
}


class No_Cache
{
public:
    void process(shared_ptr<Data> data) { Q_UNUSED(data) }
    bool empty() const { return true; }
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_CACHE_H
