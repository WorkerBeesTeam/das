#ifndef DAS_SERVER_LOG_SAVER_H
#define DAS_SERVER_LOG_SAVER_H

#include <thread>
#include <mutex>
#include <condition_variable>

namespace Das {
namespace Server {

class Log_Saver
{
    static Log_Saver* _instance;
public:
    static Log_Saver* instance();

    Log_Saver();
    ~Log_Saver();

    void stop();
    void join();

    template<typename T> bool add()
    {

    }
private:
    void run();

    bool _break_flag;

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cond;
};

} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_H
