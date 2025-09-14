#include "gatt_test.h"

#include <Das/device.h>

namespace Das::Gatt {

Test::Test()
{
    group_type_mng_.add(1, "temp", "Температура");
    device_item_type_mng_.add(1, "mi_temp", "Mi Температура", 1, 0, Device_Item_Type::RT_INPUT_REGISTERS, Device_Item_Type::SA_OFF);
    dig_mode_type_mng_.add(1, "automation", "Auto");
    param_mng_.add(1, "address_param", "Mi address", QString{}, DB::DIG_Param_Type::VT_STRING, 1);

    plugin_type_mng_->add(1, "gatt", QStringList{"address_param_id", "service"}, QStringList{"characteristic"});
    plugin_type_mng_->get_type(1)->need_it = true;

    Device dev{1, "Device #1", {{"address_param_id", 1}, {"service", "ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6"}}, 1, 3000};
    _dev = add_device(std::move(dev));

    Device_Item item{1, QString(), 1, QVariantList{"ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6"}, 0, 1};
    _item = _dev->create_item(std::move(item));
    connect(_item, &Device_Item::value_changed, this, &Test::value_changed);
    connect(_item, &Device_Item::connection_state_changed, this, &Test::connection_state_changed);

    auto sct = add_section(Section{1, "Sct1"});
    _dig = sct->add_group(DB::Device_Item_Group{1, {}, 1, 1}, 1);
    _dig->params()->add(DIG_Param{1, 1, 1}, {}, &param_mng_);
    _dig->add_item(_item);
    _dig->finalize();
}

bool Test::start(const QString& file_path)
{
    if (!load(file_path))
        return false;

    try {
        _pl_type->checker->start();
    } catch (const std::exception& e) {
        qCritical() << "Can't start plugin" << e.what();
        return false;
    }
    return true;
}

void Test::value_changed(uint32_t user_id, const QVariant& old_raw_value)
{
    qDebug() << "Value" << _item->value().toString();
}

void Test::connection_state_changed(bool state)
{
    qDebug() << "Connection state" << state;
}

} // namespace Das::Gatt
