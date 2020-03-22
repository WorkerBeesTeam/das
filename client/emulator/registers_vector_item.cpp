#include "registers_vector_item.h"
#include "device_item_table_item.h"
#include <QDebug>

RegistersVectorItem::RegistersVectorItem(Das::DB::Device_Item_Type_Manager* mng,
                                         QModbusServer* modbus_server,
                                         QModbusDataUnit::RegisterType type,
                                         const QVector<Das::Device_Item*>& items,
                                         QObject* parent): QObject(parent), type_(type), modbus_server_(modbus_server),
                                         device_item_type_manager_(mng)
{
    assign(items);
}

QVector<Das::Device_Item*> RegistersVectorItem::assign(const QVector<Das::Device_Item*>& items)
{
    QVector<Das::Device_Item*> result;
    for (const auto& ptr : items) {
        if (device_item_type_manager_->register_type(ptr->type_id()) == type_) {
            bool is_found = false;

            for (const auto& item : items_)
            {
                if (unit(ptr) == unit(item))
                {
                    is_found = true;
                    break;
                }
            }

            if (!is_found)
            {
                items_.push_back(ptr);
                result.push_back(ptr);
            }
        }
    }

    auto sort_units = [](const Das::Device_Item *a, const Das::Device_Item *b) -> bool
    {
        return unit(a) < unit(b);
    };

    std::sort(items_.begin(), items_.end(), sort_units);
    std::sort(result.begin(), result.end(), sort_units);

    return result;
}

Das::Device_Item *RegistersVectorItem::at(int i) const
{
    if (i >= this->items().size() || i < 0)
    {
        return nullptr;
    }
    return this->items_.at(i);
}

QModbusDataUnit::RegisterType RegistersVectorItem::type() const {
    return this->type_;
}

QVector<Das::Device_Item*> RegistersVectorItem::items() const {
    return this->items_;
}

Das::DB::Device_Item_Type_Manager* RegistersVectorItem::manager() const {
    return this->device_item_type_manager_;
}

QModbusServer* RegistersVectorItem::modbus_server() const {
    return this->modbus_server_;
}

QModbusDataUnit RegistersVectorItem::modbus_data_unit() const
{
    QModbusDataUnit::RegisterType registerType = this->type();
    QVector<Das::Device_Item*> units = this->items();

    if (units.size() == 0)
        return QModbusDataUnit();

    int max_unit = unit(units.back());
    auto it = units.begin();

    for (int i = 0; i <= max_unit; ++i, ++it)
    {
        int unit_i = unit(*it);
        if (unit_i < 0)
        {
            --i;
        }
        else if (it == units.end() || unit_i != i)
        {
            it = units.insert(it, nullptr);
        }
    }

    QModbusDataUnit modbus_unit(registerType, 0, units.size());

    if (registerType > QModbusDataUnit::Coils)
    {
        for (uint i = 0; i < modbus_unit.valueCount(); ++i)
        {
            if (units.at(i))
            {
                modbus_unit.setValue(i, units.at(i)->raw_value().toInt());
            }
        }
    }

    return modbus_unit;
}
