#ifndef DAS_TYPE_MANAGERS_H
#define DAS_TYPE_MANAGERS_H

#include <QVariant>
#include <map>

#include "db/sign_type.h"
#include "db/code_item.h"
#include "db/device_item_type.h"
#include "db/dig_status_type.h"
#include "db/dig_param_type.h"
#include "db/dig_mode_type.h"
#include "db/dig_status_category.h"
#include "db/dig_type.h"
#include "db/plugin_type.h"
#include "db/save_timer.h"

namespace Das {

struct DAS_LIBRARY_SHARED_EXPORT Type_Managers
{
    Sign_Manager sign_mng_;
    Status_Manager status_mng_;
    DIG_Param_Type_Manager param_mng_;
    Device_Item_Type_Manager device_item_type_mng_;
    DIG_Mode_Type_Manager dig_mode_type_mng_;
    DIG_Status_Category_Manager dig_status_category_mng_;
    DIG_Type_Manager group_type_mng_;

    std::shared_ptr<Plugin_Type_Manager> plugin_type_mng_;
};

QString DAS_LIBRARY_SHARED_EXPORT signByDevItem(const Type_Managers* mng, uint32_t dev_Device_item_type);

template<typename Container>
void item_delete_later(Container& item_list) {
    for (typename Container::value_type item: item_list)
        item->deleteLater();
    item_list.clear();
}

} // namespace Das

#endif // DAS_TYPE_MANAGERS_H
