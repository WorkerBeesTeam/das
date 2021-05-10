#ifndef DAS_REST_CONFIG_H
#define DAS_REST_CONFIG_H

#include <string>
#include <chrono>

namespace Das {
namespace Rest {

struct Config
{
    static Config& instance()
    {
        static Config config;
        return config;
    }

    int thread_count_ = 3;
    std::string address_ = "localhost",
                port_ = "8123",
                base_path_,
                _blob_dir_path = "/opt/das/blob";

    std::chrono::seconds _token_timeout = std::chrono::seconds{3600};
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CONFIG_H
