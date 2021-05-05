#ifndef DAS_SERVER_LOG_SAVER_SAVER_H
#define DAS_SERVER_LOG_SAVER_SAVER_H

#include <iostream>
#include <type_traits>
#include <queue>

#include "log_saver_cache.h"

namespace Das {
namespace Log_Saver {

template<typename T>
class Saver : public Saver_Base
{
public:
    using Cache_Value_Type = typename Cache_Type<T>::Type;
    using Saver_Cache_Type = typename conditional<Cache_Type<T>::Is_Comparable::value,
                                         Saver_Cache<Cache_Value_Type>, No_Cache>::type;
    Saver_Cache_Type& cache() { return _cache; }

    virtual ~Saver() = default;

    void set_after_insert_log_callback(function<void(const vector<T>&)> cb)
    {
        _after_insert_log_cb = move(cb);
    }

    template<template<typename...> class Container>
    void add(uint32_t scheme_id, const Container<T>& data)
    {
        for (const T& item: data)
        {
            if (item.scheme_id() == 0 || item.scheme_id() != scheme_id)
                cout << "Attempt to add non scheme item " << item.scheme_id() << " s " << scheme_id << endl;

            if constexpr (is_same<T, Log_Value_Item>::value)
                if (!item.need_to_save())
                    continue;

            _data.push(item);
        }
    }

    template<template<typename...> class Container>
    void add_cache(uint32_t scheme_id, const Container<T>& data, chrono::seconds time_in_cache)
    {
        static_assert(Cache_Type<T>::Is_Comparable::value, "Only comparable type have cache");
        _cache.add_data(scheme_id, data, time_in_cache);
    }

    bool empty(bool cache_force = false) const override
    {
        if constexpr (Cache_Type<T>::Is_Comparable::value)
        {
            if (!_cache.empty() && _cache.is_ready_to_save(cache_force))
                return false;
        }
        else
            Q_UNUSED(cache_force);
        return _data.empty();
    }

    time_point get_save_time() const override
    {
        if constexpr (Cache_Type<T>::Is_Comparable::value)
            if (_data.empty())
                return _cache.get_oldest_time();
        return _data.empty() ? time_point::max() : clock::now();
    }

    shared_ptr<Data> get_data_pack(size_t max_pack_size = 100, bool cache_force = false) override
    {
        if constexpr (Cache_Type<T>::Is_Comparable::value)
        {
            if (_cache.is_ready_to_save(cache_force))
                return _cache.get_data_pack(max_pack_size);
        }
        else
            Q_UNUSED(cache_force)

        vector<T> pack;
        while(max_pack_size-- && !_data.empty())
        {
            pack.push_back(move(_data.front()));
            _data.pop();
        }
        return make_shared<Data_Impl<T>>(Data::Log, move(pack));
    }

private:

    void process_data_pack(shared_ptr<Data> data) override
    {
        if (!data)
            return;

        if (data->type() == Data::Log)
            process(data);
        else
        {
            if constexpr (Cache_Type<T>::Is_Comparable::value)
                _cache.process(data);
        }
    }

    void process(shared_ptr<Data> data)
    {
        auto t_data = static_pointer_cast<Data_Impl<T>>(data);
        vector<T>& pack = t_data->_data;

        QVariantList values_pack;
        for (const T& item: pack)
            values_pack += T::to_variantlist(item);

        Base& db = Base::get_thread_local_instance();

        auto table = db_table<T>();
        const QString sql = get_custom_q_array(table, values_pack.size() / table.field_names().size());
        if (!db.exec(sql, values_pack).isActive())
        {
            if (Log_Saver_Log().isCriticalEnabled())
            {
                for (int i = 0; i < values_pack.size(); )
                {
                    auto dbg = qCritical(Log_Saver_Log).noquote() << "Lost log " << typeid(T).name();
                    for (const QString& name: table.field_names())
                    {
                        dbg << name << '=' << values_pack.at(i).toString() << ' ';
                        ++i;
                    }
                }
            }
            // TODO: Try to insert each separately. With check duplicate?

            Saver_Base::save_dump_to_file(typeid(T).hash_code(), values_pack);
            // TODO: Use load_dump_file in constructor?
        }
        else if (_after_insert_log_cb)
            _after_insert_log_cb(pack);
    }

    QString get_custom_q_array(const Table& table, int row_count) const
    {
        // " + (force ? QString("IGNORE ") : QString()) + "
        return "INSERT IGNORE INTO " +
                table.name() + '(' + table.field_names().join(',') + ") VALUES" +
                Base::get_q_array(table.field_names().size(), row_count);
    }

    queue<T> _data;
    Saver_Cache_Type _cache;
    function<void(const vector<T>&)> _after_insert_log_cb;
};

} // namespace Log_Saver
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_SAVER_H
