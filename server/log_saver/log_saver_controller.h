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
    static Controller* _instance;
public:
    static Controller* instance();

    Controller(int thread_count = 5);
    ~Controller();

    void stop();
    void join();

    template<typename T>
    bool add(uint32_t scheme_id, QVector<T>& data)
    {
        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
        {
            try
            {
                {
                    lock_guard lock(_mutex);
                    static_pointer_cast<Saver<T>>(it->second)->add(scheme_id, data);
                }

                _cond.notify_one();
                return true;
            }
            catch(...) {}
        }

        return false;
    }

    void set_cache_data(QVector<T>&& data, uint32_t scheme_id)
    {
        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
            return static_pointer_cast<Saver<T>>(it->second)->set_cache_data(move(data), scheme_id);
    }

    template<typename T, template<typename...> class Container = QVector, typename K = typename Helper<T>::Value_Type>
    Container<K> get_cache_data(uint32_t scheme_id)
    {
        auto it = _savers.find(typeid(T));
        if (it != _savers.cend())
            return static_pointer_cast<Saver<T>>(it->second)->template get_cache_data<Container, K>(scheme_id);
        return {};
    }

    void erase_empty_cache();

    void set_devitem_values(QVector<Log_Value_Item>&& data, uint32_t scheme_id);
    QVector<Device_Item_Value> get_devitem_values(uint32_t scheme_id);

    void set_statuses(QVector<Log_Status_Item>&& data, uint32_t scheme_id);
    set<DIG_Status> get_statuses(uint32_t scheme_id);

private:
    void run();
    bool empty() const;
    bool empty_data() const;
    bool empty_cache() const;
    bool empty_impl(bool(Saver_Base::*empty_func)() const) const;
    time_point get_oldest_cache_time() const;
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

inline Log_Saver::Controller* instance()
{
    return Log_Saver::Controller::instance();
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_CONTROLLER_H
