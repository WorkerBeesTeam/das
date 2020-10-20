#ifndef DAS_SERVER_LOG_SAVER_CACHE_H
#define DAS_SERVER_LOG_SAVER_CACHE_H

#include <Helpz/db_base.h>

#include "log_saver_base.h"
#include "log_saver_cache_data.h"

namespace Das {
namespace Server {
namespace Log_Saver {

template<typename T>
class Saver_Cache : public Saver_Base
{
public:
    virtual ~Saver_Cache() = default;

    void set_cache_data(QVector<T>&& data, uint32_t scheme_id)
    {
        boost::shared_lock lock(_cache_mutex);
        Cache_Data<T>& cache = get_cache(scheme_id, lock);
        cache.set_data(move(data), get_save_time());
    }

    template<template<typename...> class Container = QVector>
    Container<T> get_cache_data(uint32_t scheme_id)
    {
        boost::shared_lock lock(_cache_mutex);
        auto it = _scheme_cache.find(scheme_id);
        if (it == _scheme_cache.cend())
            return {};

        return it->second.template get_data<Container>();
    }

protected:

    template<typename Log_Type, template<typename...> class Container>
    void add_cache(uint32_t scheme_id, const Container<Log_Type>& data)
    {
        boost::shared_lock lock(_cache_mutex);
        Cache_Data<T>& cache = get_cache(scheme_id, lock);

        const time_point save_time_point = get_save_time();
        cache.add(data, save_time_point);
        if (_oldest_cache_time.load().time_since_epoch() == clock::duration::zero())
            _oldest_cache_time = save_time_point;
    }

    bool empty_cache() const override
    {
        boost::shared_lock lock(_cache_mutex);
        for (const auto& it: _scheme_cache)
            if (!it.second.empty())
                return false;
        return true;
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
        return make_shared<Data_Impl<T>>(Data::Cache, move(pack));
    }

    void process_cache(shared_ptr<Data> data) override
    {
        auto t_data = static_pointer_cast<Data_Impl<T>>(data);
        process_cache(t_data->_data);
    }

    void process_cache(vector<T>& pack) { process_cache_impl(pack); }
    void process_cache_impl(vector<T>& pack)
    {
        // UPDATE dig_param_value SET timestamp_msecs=? AND user_id=? AND value=?
        // WHERE scheme_id=? AND group_param_id=? AND timestamp_msecs<=?

        Base& db = Base::get_thread_local_instance();

        Table table = db_table<T>();
        const vector<uint32_t> updating_fields = Cache_Helper<T>::get_values_fields();
        const vector<uint32_t> compared_fields = Cache_Helper<T>::get_compared_fields();
        const QString where = get_update_where<T>(table, compared_fields);
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
                // TODO: SELECT before insert for reasons when CLIENT_FOUND_ROWS=1 opt is not set.
                if (!db.insert(table, T::to_variantlist(item)))
                {
                    // TODO: do something
                }
            }
        }
    }

    time_point get_save_time() const
    {
        return clock::now() + 15s;
    }

    Cache_Data<T>& get_cache(uint32_t scheme_id, boost::shared_lock<boost::shared_mutex>& shared_lock)
    {
        auto it = _scheme_cache.find(scheme_id);
        if (it == _scheme_cache.end())
        {
            shared_lock.unlock();
            {
                lock_guard lock(*shared_lock.mutex());
                it = _scheme_cache.find(scheme_id);
                if (it == _scheme_cache.end())
                    _scheme_cache.emplace(scheme_id, Cache_Data<T>{});
            }
            shared_lock.lock();
            return get_cache(scheme_id, shared_lock);
        }
        return it->second;
    }

private:
    mutable boost::shared_mutex _cache_mutex;
    map<uint32_t/*scheme_id*/, Cache_Data<T>> _scheme_cache;
    atomic<time_point> _oldest_cache_time;
};

template<>
inline void Saver_Cache<DIG_Status>::process_cache(vector<DIG_Status>& pack)
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
        process_cache_impl(pack);
}


class No_Cache : public Saver_Base
{
public:
    virtual ~No_Cache() = default;
protected:
    void process_cache(shared_ptr<Data> data) override { Q_UNUSED(data) }
    void erase_empty_cache() override {}
    bool empty_cache() const override { return true; }
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_CACHE_H
