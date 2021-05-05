#ifndef DAS_SERVER_LOG_SAVER_CACHE_DATA_H
#define DAS_SERVER_LOG_SAVER_CACHE_DATA_H

#include <atomic>
#include <boost/thread/shared_mutex.hpp>

#include <QVector>

#include <plus/das/log_saver_helper.h>

namespace Das {
namespace Log_Saver {

template<typename T>
struct Cache_Item
{
    Cache_Item(time_point save_time, const T& item) : _save_time(save_time), _item(item) {}
    time_point _save_time;
    T _item;
};

template<typename T>
class Cache_Data
{
public:
    Cache_Data() :
        _oldest_cache_time{time_point::max()}
    {}
    Cache_Data(Cache_Data<T>&& o) :
        _oldest_cache_time{o._oldest_cache_time.load()},
        _data{move(o._data)} {}

    bool empty() const
    {
        boost::shared_lock lock(_mutex);
        return _data.empty();
    }

    template<typename Log_Type, template<typename...> class Container = QVector>
    void add(const Container<Log_Type>& data, const time_point save_time_point)
    {
        lock_guard lock(_mutex);
        if (_data.empty())
            _oldest_cache_time = save_time_point;

        for (const Log_Type& item: data)
            add_item(item, save_time_point);
    }

    static bool is_item_less(const Cache_Item<T>& cache_item, const typename Type_Base<T>::Type& item)
    {
        return Cache_Helper<T>::is_item_less(cache_item._item, item);
    }

    time_point get_ready_to_save_data(vector<T>& container, time_point time, size_t max_pack_size,
                                      bool& is_cache_empty)
    {
        time_point oldest_cache_time = _oldest_cache_time.load();
        if (container.size() >= max_pack_size)
            return oldest_cache_time;
        if (oldest_cache_time > time)
            return time_point::max();

        oldest_cache_time = time_point::max();

        lock_guard lock(_mutex);
        for (auto it = _data.begin(); it != _data.end();)
        {
            if (container.size() < max_pack_size && it->_save_time < time)
            {
                container.push_back(move(it->_item));
                it = _data.erase(it);
            }
            else
            {
                if (oldest_cache_time > it->_save_time)
                    oldest_cache_time = it->_save_time;
                ++it;
            }
        }

        if (!is_cache_empty && _data.empty())
            is_cache_empty = true;

        _oldest_cache_time = oldest_cache_time;
        return oldest_cache_time;
    }

    void set_data(QVector<T>&& data, const time_point save_time_point)
    {
        lock_guard lock(_mutex);
        _oldest_cache_time = save_time_point;
        _data.clear();
        for (T& item: data)
            add_item(item, save_time_point);
    }

    template<template<typename...> class Container = QVector>
    Container<T> get_data() const
    {
        Container<T> data;

        boost::shared_lock lock(_mutex);
        for (const Cache_Item<T>& item: _data)
            data.insert( data.end(), item._item );
        return data;
    }

private:

    template<typename Log_Type>
    void add_item(const Log_Type& item, const time_point save_time_point)
    {
        auto it = lower_bound(_data.begin(), _data.end(), item, is_item_less);

        if (it != _data.end() && Cache_Helper<T>::is_item_equal(item, it->_item))
        {
            if (it->_item.timestamp_msecs() < item.timestamp_msecs())
            {
                it->_item = item;
                it->_save_time = save_time_point;
            }
        }
        else
            _data.emplace(it, save_time_point, T{item});
    }

    atomic<time_point> _oldest_cache_time;

    mutable boost::shared_mutex _mutex;
    vector<Cache_Item<T>> _data;
};

} // namespace Log_Saver
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_CACHE_DATA_H
