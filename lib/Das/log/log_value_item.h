#ifndef DAS_DB_LOG_VALUE_ITEM_H
#define DAS_DB_LOG_VALUE_ITEM_H

#include <Das/db/device_item_value.h>

namespace Das {
namespace DB {

class Log_Value_Item : public Device_Item_Value_Base
{
    HELPZ_DB_META(Log_Value_Item, "log_value", "lv", DEV_ITEM_DB_META_ARGS)
public:
    using Device_Item_Value_Base::Device_Item_Value_Base;
    explicit Log_Value_Item(const Device_Item_Value_Base& o) : Device_Item_Value_Base{o} {}
    explicit Log_Value_Item(Device_Item_Value_Base&& o) : Device_Item_Value_Base{std::move(o)} {}

    bool need_to_save() const;
    void set_need_to_save(bool state);
};

} // namespace DB
} // namespace Das

using Log_Value_Item = Das::DB::Log_Value_Item;
Q_DECLARE_METATYPE(Log_Value_Item)

#endif // DAS_DB_LOG_VALUE_ITEM_H
