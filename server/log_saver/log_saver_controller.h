#ifndef DAS_SERVER_LOG_SAVER_CONTROLLER_H
#define DAS_SERVER_LOG_SAVER_CONTROLLER_H

#include <set>
#include <typeindex>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "log_saver_def.h"
#include "log_saver.h"

namespace Das {
namespace Server {
namespace Log_Saver {

class Controller
{
public:
    Controller(size_t thread_count = 5, size_t max_pack_size = 100, chrono::seconds time_in_cache = 15s);
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
                    static_pointer_cast<Saver<T>>(it->second)->add(scheme_id, data, _time_in_cache);
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
            return static_pointer_cast<Saver<T>>(it->second)->cache().set_data(move(data), scheme_id, _time_in_cache);
    }

    template<typename T, template<typename...> class Container = QVector>
    Container<typename Cache_Type<T>::Type> get_cache_data(uint32_t scheme_id)
    {
        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
            return static_pointer_cast<Saver<T>>(it->second)->cache().template get_data<Container>(scheme_id);
        return {};
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

    size_t _max_pack_size;
    chrono::seconds _time_in_cache;

    using Saver_Map = map<type_index, shared_ptr<Saver_Base>>;
    Saver_Map _savers;
    Saver_Map::const_iterator _current_saver;
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_CONTROLLER_H
