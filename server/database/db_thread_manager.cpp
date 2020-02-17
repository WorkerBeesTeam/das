#include "db_thread_manager.h"

namespace Das {
namespace DB {

Thread_Manager::Thread_Manager(Helpz::DB::Connection_Info info) :
    db_thread_(info, 5, 90),
    db_log_thread_(info, 1)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

Thread_Manager::~Thread_Manager()
{
}

Helpz::DB::Thread* Thread_Manager::thread()
{
    return &db_thread_;
}

Helpz::DB::Thread *Thread_Manager::log_thread()
{
    return &db_log_thread_;
}

std::shared_ptr<global> Thread_Manager::get_db()
{
    auto thread_id = std::this_thread::get_id();
    {
        boost::shared_lock lock_shared(mutex_);
        auto it = db_list_.find(thread_id);
        if (it != db_list_.cend())
        {
            return it->second;
        }
    }

    std::lock_guard lock(mutex_);
    auto it = db_list_.emplace(thread_id, global::open("some_name", thread()));
    assert(it.second);
    return it.first->second;
}

} // namespace database
} // namespace Das
