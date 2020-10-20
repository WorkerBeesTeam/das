#ifndef DAS_DB_LOG_MODE_H
#define DAS_DB_LOG_MODE_H

#include <Das/db/dig_mode.h>

namespace Das {
namespace DB {

class Log_Mode_Item : public DIG_Mode_Base
{
    HELPZ_DB_META(Log_Mode_Item, "log_mode", "lm", DIG_MODE_DB_META_ARGS)
public:
    using DIG_Mode_Base::DIG_Mode_Base;
};

} // namespace DB
} // namespace Das

using Log_Mode_Item = Das::DB::Log_Mode_Item;
Q_DECLARE_METATYPE(Log_Mode_Item)

#endif // DAS_DB_LOG_MODE_H
