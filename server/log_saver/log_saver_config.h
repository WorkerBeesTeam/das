#ifndef LOG_SAVER_CONFIG_H
#define LOG_SAVER_CONFIG_H

#include "log_saver_def.h"

namespace Das {
namespace Server {
namespace Log_Saver {

struct Config
{
    static Config& get()
    {
        static Config config;
        return config;
    }

    size_t _thread_count = 5;
    size_t _max_pack_size = 100;
    size_t _time_in_cache_sec = 15; // 15 seconds
    size_t _layer_min_for_now_minute = 5; // 5 minute
    size_t _layer_text_max_count = 32;
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // LOG_SAVER_CONFIG_H
