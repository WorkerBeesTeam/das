#ifndef DAS_DB_THREAD_MANAGER_H
#define DAS_DB_THREAD_MANAGER_H

#include <thread>
#include <boost/thread/shared_mutex.hpp>

#include "db_scheme.h"

namespace Das {
namespace Database {

class Thread_Manager
{
public:
    Thread_Manager(Helpz::Database::Connection_Info info);
    ~Thread_Manager();

    Helpz::Database::Thread* thread();
    Helpz::Database::Thread* log_thread();
    std::shared_ptr<global> get_db();
private:
    boost::shared_mutex mutex_;
    std::map<std::thread::id, std::shared_ptr<global>> db_list_;

    Helpz::Database::Thread db_thread_;
    Helpz::Database::Thread db_log_thread_;
};

} // namespace Database
} // namespace Das

#endif // DAS_DB_THREAD_MANAGER_H
