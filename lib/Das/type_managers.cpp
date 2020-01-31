#include <QDebug>
#include "type_managers.h"

namespace Das {

QString signByDevItem(const Type_Managers *mng, uint32_t dev_Device_item_type) {
    return mng->sign_mng_.name( mng->device_item_type_mng_.sign_id(dev_Device_item_type) );
}

} // namespace Das
