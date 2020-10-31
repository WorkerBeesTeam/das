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
    explicit Log_Mode_Item(const DIG_Mode_Base& o) : DIG_Mode_Base{o} {}
    explicit Log_Mode_Item(DIG_Mode_Base&& o) : DIG_Mode_Base{std::move(o)} {}
};

} // namespace DB
} // namespace Das

using Log_Mode_Item = Das::DB::Log_Mode_Item;
Q_DECLARE_METATYPE(Log_Mode_Item)

#endif // DAS_DB_LOG_MODE_H
