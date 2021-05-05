#ifndef DAS_LOG_SYNCHRONIZER_H
#define DAS_LOG_SYNCHRONIZER_H

#include <map>
#include <set>
#include <vector>
#include <ctime>

#include <QIODevice>

#include <Helpz/db_table.h>

#include <Das/log/log_type.h>
#include <Das/log/log_pack.h>
#include <plus/das/scheme_info.h>

#include "base_synchronizer.h"

namespace Das {
namespace Ver {
namespace Server {

using namespace Das::Server;

class Log_Sync_Base
{
public:
    virtual ~Log_Sync_Base() = default;
    virtual bool process(const Scheme_Info& scheme, QIODevice& data_dev, bool skip_cache) = 0;
};

class Log_Synchronizer
{
public:
    Log_Synchronizer();

    std::set<uint8_t> get_type_set() const;
    bool process(Log_Type_Wrapper type_id, QIODevice* data_dev, const Scheme_Info &scheme, bool skip_cache = false);

private:
    std::map<uint8_t, std::shared_ptr<Log_Sync_Base>> _sync;
};

} // namespace Server
} // namespace Ver
} // namespace Das

#endif // DAS_LOG_SYNCHRONIZER_H
