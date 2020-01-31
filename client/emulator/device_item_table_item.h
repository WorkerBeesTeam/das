#ifndef DEVICEITEMTABLEITEM_H
#define DEVICEITEMTABLEITEM_H

#include "devices_table_item.h"
#include <Das/device_item.h>
#include <Das/type_managers.h>
#include <QModbusServer>

class Device_ItemTableItem : public DevicesTableItem
{
    Das::Database::Device_Item_Type_Manager* device_item_type_manager_;
    QModbusServer* modbus_server_;
    Das::Device_Item* dev_item_;
public:
    Device_ItemTableItem(Das::Database::Device_Item_Type_Manager *mng, QModbusServer* modbus_server, Das::Device_Item* item, DevicesTableItem* parent = nullptr);
    ~Device_ItemTableItem() override = default;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModbusServer* modbus_server() const;

    bool is_favorite() const;
    void set_is_favorite(bool is_favorite);
};

int unit(const Das::Device_Item *a);

#endif // DEVICEITEMTABLEITEM_H
