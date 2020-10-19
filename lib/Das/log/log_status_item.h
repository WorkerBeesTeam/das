#ifndef DAS_DB_LOG_STATUS_ITEM_H
#define DAS_DB_LOG_STATUS_ITEM_H

#include <Helpz/db_meta.h>

#include <Das/db/dig_status.h>

namespace Das {
namespace DB {

class Log_Status_Item : public DIG_Status_Base
{
    HELPZ_DB_META(Log_Status_Item, "log_status", "ls", DIG_STATUS_DB_META_ARGS, DB_A(direction), DB_A(scheme_id))
public:
    using DIG_Status_Base::DIG_Status_Base;
};

} // namespace DB
} // namespace Das

using Log_Status_Item = Das::DB::Log_Status_Item;
Q_DECLARE_METATYPE(Log_Status_Item)

#endif // DAS_DB_LOG_STATUS_ITEM_H
