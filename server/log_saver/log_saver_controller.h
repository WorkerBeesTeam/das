#ifndef DAS_SERVER_LOG_SAVER_CONTROLLER_H
#define DAS_SERVER_LOG_SAVER_CONTROLLER_H

#include <set>
#include <typeindex>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "log_saver_config.h"
#include "log_saver.h"

namespace Das {
namespace Server {
namespace Log_Saver {

class Controller
{
public:
    Controller();
    ~Controller();

    void stop();
    void join();

    template<typename T>
    bool add(uint32_t scheme_id, const QVector<T>& data)
    {
        if (_break_flag)
            return false;

        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
        {
            try
            {
                {
                    lock_guard lock(_mutex);
                    static_pointer_cast<Saver<T>>(it->second)->add(scheme_id, data,
                                                                   chrono::seconds{Config::get()._time_in_cache_sec});
                }

                _cond.notify_one();
                return true;
            }
            catch(...) {}
        }

        return false;
    }

    template<typename T>
    void set_cache_data(QVector<typename Cache_Type<T>::Type>&& data, uint32_t scheme_id)
    {
        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
            return static_pointer_cast<Saver<T>>(it->second)->cache().set_data(move(data), scheme_id,
                                                                chrono::seconds{Config::get()._time_in_cache_sec});
    }

    template<typename T, template<typename...> class Container = QVector>
    Container<typename Cache_Type<T>::Type> get_cache_data(uint32_t scheme_id)
    {
        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
            return static_pointer_cast<Saver<T>>(it->second)->cache().template get_data<Container>(scheme_id);
        return {};
    }

    template<typename T>
    void set_after_insert_log_callback(function<void(const vector<T>&)> cb)
    {
        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
            static_pointer_cast<Saver<T>>(it->second)->set_after_insert_log_callback(move(cb));
    }

private:
    void run();
    bool empty() const;
    time_point get_oldest_save_time() const;
    shared_ptr<Saver_Base> get_saver();

    template<typename T>
    void add_saver();

    bool _break_flag;
    mutex _mutex;
    condition_variable _cond;

    vector<thread> _threads;

    using Saver_Map = map<type_index, shared_ptr<Saver_Base>>;
    Saver_Map _savers;
    Saver_Map::const_iterator _current_saver;
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_CONTROLLER_H
