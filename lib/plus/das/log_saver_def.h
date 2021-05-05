#ifndef DAS_LOG_SAVER_DEF_H
#define DAS_LOG_SAVER_DEF_H

#include <memory>
#include <chrono>
#include <vector>
#include <map>

namespace Das {
namespace Log_Saver {

using namespace std;
using namespace chrono_literals;

using clock = chrono::system_clock;
using time_point = clock::time_point;

} // namespace Log_Saver
} // namespace Das

#endif // DAS_LOG_SAVER_DEF_H
