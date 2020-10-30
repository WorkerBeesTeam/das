#ifndef DAS_DB_LOG_PARAM_ITEM_H
#define DAS_DB_LOG_PARAM_ITEM_H

#include <Helpz/db_meta.h>

#include <Das/log/log_base_item.h>
#include <Das/db/dig_param_value.h>

namespace Das {
namespace DB {

class Log_Param_Item : public DIG_Param_Value_Base2
{
    HELPZ_DB_META(Log_Param_Item, "log_param", "lp", DIG_PARAM_VALUE_DB_META_ARGS)
public:
    using DIG_Param_Value_Base2::DIG_Param_Value_Base2;
};

} // namespace DB
} // namespace Das

using Log_Param_Item = Das::DB::Log_Param_Item;
Q_DECLARE_METATYPE(Log_Param_Item)

#endif // DAS_DB_LOG_PARAM_ITEM_H
