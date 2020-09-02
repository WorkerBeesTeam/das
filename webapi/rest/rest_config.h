#ifndef DAS_REST_CONFIG_H
#define DAS_REST_CONFIG_H

#include <string>

namespace Das {
namespace Rest {

struct Config
{
    int thread_count_;
    std::string address_, port_, base_path_;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CONFIG_H
