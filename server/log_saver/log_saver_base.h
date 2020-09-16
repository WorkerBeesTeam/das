#ifndef DAS_SERVER_LOG_SAVER_BASE_H
#define DAS_SERVER_LOG_SAVER_BASE_H

#include "log_saver_data.h"

namespace Das {
namespace Server {
namespace Log_Saver {

class Saver_Base
{
public:
    virtual ~Saver_Base() = default;

    bool empty() const { return empty_data() && empty_cache(); }
    virtual bool empty_data() const = 0;
    virtual bool empty_cache() const = 0;
    virtual shared_ptr<Data> get_data_pack() = 0;
    virtual void process_data_pack(shared_ptr<Data> data) = 0;
    virtual void erase_empty_cache() = 0;
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_BASE_H
