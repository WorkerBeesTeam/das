#include "log_saver.h"

namespace Das {
namespace Server {

using namespace std;

/*static*/ Log_Saver* Log_Saver::_instance = nullptr;
Log_Saver *Log_Saver::instance()
{
    return _instance;
}

Log_Saver::Log_Saver() :
    _break_flag(false)
{
    _instance = this;

    _thread = thread(&Log_Saver::run, this);
}

Log_Saver::~Log_Saver()
{
    _instance = nullptr;
}

void Log_Saver::stop()
{
    {
        lock_guard lock(_mutex);
        _break_flag = true;
    }
    _cond.notify_one();
}

void Log_Saver::join()
{
    if (_thread.joinable())
        _thread.join();
}

void Log_Saver::run()
{
    unique_lock lock(_mutex, defer_lock);

    while (true)
    {
        lock.lock();
        _cond.wait(lock, [this]() { return !_break_flag; });

        if (_break_flag)
        {
            break;
        }
    }
}

} // namespace Server
} // namespace Das
