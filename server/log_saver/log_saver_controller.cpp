#include "log_saver_controller.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace std;
using namespace Helpz::DB;

Controller::Controller(size_t thread_count, size_t max_pack_size, chrono::seconds time_in_cache) :
    _break_flag(false),
    _max_pack_size(max_pack_size),
    _time_in_cache(time_in_cache)
{
    add_saver<Log_Value_Item>();
    add_saver<Log_Event_Item>();
    add_saver<Log_Param_Item>();
    add_saver<Log_Status_Item>();
    add_saver<Log_Mode_Item>();

    _current_saver = _savers.cbegin();

    while (thread_count--)
        _threads.emplace_back(&Controller::run, this);
}

Controller::~Controller()
{
    stop();
    join();
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

void Controller::run()
{
    time_point timeout_time;
    unique_lock lock(_mutex, defer_lock);

    while (true)
    {
        lock.lock();
        if (!_break_flag)
        {
            timeout_time = get_oldest_save_time();
            if (timeout_time == time_point::max())
                _cond.wait(lock, [this]() { return _break_flag || !empty(); });
            else if (timeout_time > clock::now())
                _cond.wait_until(lock, timeout_time);
        }

        if (!empty())
        {
            shared_ptr<Data> data;
            shared_ptr<Saver_Base> saver = get_saver();
            if (saver)
                data = saver->get_data_pack(_max_pack_size, _break_flag);

            lock.unlock();
            _cond.notify_one();

            if (saver && data)
                saver->process_data_pack(data);
        }
        else if (_break_flag)
            break;
        else
        {
            lock.unlock();
        }
    }
}

bool Controller::empty() const
{
    for (const auto& saver: _savers)
        if (!saver.second->empty())
            return false;
    return true;
}

time_point Controller::get_oldest_save_time() const
{
    time_point oldest_time = time_point::max(), saver_time, now = clock::now();
    for (const auto& saver: _savers)
    {
        saver_time = saver.second->get_save_time();
        if (oldest_time > saver_time)
            oldest_time = saver_time;
        if (saver_time <= now)
            break;
    }
    return oldest_time;
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
