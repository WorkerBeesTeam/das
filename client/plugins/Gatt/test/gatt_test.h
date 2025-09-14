#ifndef DAS_GATT_TEST_H
#define DAS_GATT_TEST_H

#include "checker_test_base.h"

namespace Das::Gatt {

class Test : public Checker_Test_Base
{
public:
    Test();
    bool start(const QString& file_path);

private slots:
    void value_changed(uint32_t user_id, const QVariant& old_raw_value);
    void connection_state_changed(bool state);

private:
    Device* _dev;
    Device_Item* _item;
    Device_item_Group* _dig;
};

} // namespace Das::Gatt

#endif // DAS_GATT_TEST_H
