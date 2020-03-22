#ifndef DAS_DB_THREAD_MANAGER_H
#define DAS_DB_THREAD_MANAGER_H

#include <thread>
#include <boost/thread/shared_mutex.hpp>

#include "db_scheme.h"

namespace Das {
namespace DB {

class Thread_Manager
{
public:
    Thread_Manager(Helpz::DB::Connection_Info info);
    ~Thread_Manager();

    Helpz::DB::Thread* thread();
    Helpz::DB::Thread* log_thread();
    std::shared_ptr<global> get_db();
private:
    boost::shared_mutex mutex_;
    std::map<std::thread::id, std::shared_ptr<global>> db_list_;

    Helpz::DB::Thread db_thread_;
    Helpz::DB::Thread db_log_thread_;
};

} // namespace DB
} // namespace Das

#endif // DAS_DB_THREAD_MANAGER_H
