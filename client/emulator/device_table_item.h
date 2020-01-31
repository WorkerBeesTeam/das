#ifndef DEVICETABLEITEM_H
#define DEVICETABLEITEM_H

#include <QModbusServer>
#include <Das/device.h>
#include <Das/type_managers.h>
#include <vector>
#include <map>
#include "devices_table_item.h"

class RegisterTableItem;

class DeviceTableItem : public DevicesTableItem
{
    Q_OBJECT

    bool enabled_;
    QModbusServer* modbus_server_;

    RegisterTableItem* get_child_register_item_by_type(QModbusDataUnit::RegisterType type) const;

    void init_by_register_type(Das::Device_Item_Type_Manager *mng, QModbusServer* modbus_server, Das::Device *device, QModbusDataUnit::RegisterType registerType);
    void update_modbus_server_map();

    QModbusDataUnitMap generate_modbus_data_unit_map() const;

    int get_real_row(int row) const;
signals:
    void data_changed(DeviceTableItem*);
public slots:
    void update_table_values(QModbusDataUnit::RegisterType type, int address, int size);
public:
    DeviceTableItem(Das::Device_Item_Type_Manager *mng, QModbusServer* modbus_server, Das::Device *device, DevicesTableItem* parent = nullptr);
    ~DeviceTableItem() override = default;

    void assign(Das::Device *device);
    int childCount() const override;
    DevicesTableItem* child(int row) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool isUse() const noexcept;
    void toggle() noexcept;
};

#endif // DEVICETABLEITEM_H
