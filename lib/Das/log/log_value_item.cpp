#include "log_value_item.h"

namespace Das {
namespace DB {

bool Log_Value_Item::need_to_save() const { return flag_; }
void Log_Value_Item::set_need_to_save(bool state) { flag_ = state; }

} // namespace DB
} // namespace Das
