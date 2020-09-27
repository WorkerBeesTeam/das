#include "log_saver_controller.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace std;
using namespace Helpz::DB;

/*static*/ Controller* Controller::_instance = nullptr;
Controller *Controller::instance()
{
    return _instance;
}

Controller::Controller(int thread_count) :
    _break_flag(false)
{
    add_saver<Log_Value_Item>();
    add_saver<Log_Event_Item>();
    add_saver<Log_Param_Item>();
    add_saver<Log_Status_Item>();
    add_saver<Log_Mode_Item>();

    _current_saver = _savers.cbegin();

    _instance = this;

    for (int i = 0; i < thread_count; ++i)
        _threads.emplace_back(&Controller::run, this);
}

Controller::~Controller()
{
    stop();
    join();

    _instance = nullptr;
}

void Controller::stop()
{
    {
        lock_guard lock(_mutex);
        _break_flag = true;
    }
    _cond.notify_all();
}

void Controller::join()
{
    for (thread& t: _threads)
        if (t.joinable())
            t.join();
    typeid(thread).hash_code();
}

void Controller::erase_empty_cache()
{
    for (const auto& saver: _savers)
        saver.second->erase_empty_cache();
}

void Controller::set_devitem_values(QVector<Log_Value_Item> &&data)
{

}

QVector<Device_Item_Value> Controller::get_devitem_values(uint32_t scheme_id)
{

}

void Controller::set_statuses(QVector<Log_Status_Item> &&data)
{

}

set<DIG_Status> Controller::get_statuses(uint32_t scheme_id)
{
    return get_cache_data<Log_Status_Item, set, DIG_Status>(scheme_id);
}

void Controller::run()
{
    unique_lock lock(_mutex, defer_lock);

    while (true)
    {
        lock.lock();
        if (empty_data() && !empty_cache())
            _cond.wait_until(lock, get_oldest_cache_time());
        else
            _cond.wait(lock, [this]() { return _break_flag || !empty(); });

        if (!empty())
        {
            shared_ptr<Data> data;
            shared_ptr<Saver_Base> saver = get_saver();
            if (saver)
                data = saver->get_data_pack();

            lock.unlock();
            _cond.notify_one();

            if (saver && data)
                saver->process_data_pack(data);
        }
        else if (_break_flag)
            break;
        else
            lock.unlock();
    }
}

bool Controller::empty() const
{
    return empty_impl(&Saver_Base::empty);
}

bool Controller::empty_data() const
{
    return empty_impl(&Saver_Base::empty_data);
}

bool Controller::empty_cache() const
{
    return empty_impl(&Saver_Base::empty_cache);
}

bool Controller::empty_impl(bool (Saver_Base::*empty_func)() const) const
{
    for (const auto& saver: _savers)
        if (!((*saver.second).*empty_func)())
            return false;
    return true;
}

time_point Controller::get_oldest_cache_time() const
{
    return {};
}

shared_ptr<Saver_Base> Controller::get_saver()
{
    shared_ptr<Saver_Base> saver;

    auto end = _current_saver;
    do
    {
        if (_current_saver == _savers.cend())
            _current_saver = _savers.cbegin();
        if (!_current_saver->second->empty())
            saver = _current_saver->second;
        ++_current_saver;
    }
    while (!saver && _current_saver != end);
    return saver;
}

template<typename T>
void Controller::add_saver()
{
    _savers.emplace(typeid(T), make_shared<Saver<T>>());
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das
