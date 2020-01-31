#ifndef REGISTERS_VECTOR_ITEM_H
#define REGISTERS_VECTOR_ITEM_H

#include <QObject>
#include <Das/device_item.h>
#include <Das/type_managers.h>
#include <QModbusDataUnit>
#include <QModbusServer>

class RegistersVectorItem : public QObject {
    Q_OBJECT

    QVector<Das::Device_Item*> items_;
    QModbusDataUnit::RegisterType type_;
    QModbusServer* modbus_server_;
    Das::Database::Device_Item_Type_Manager* device_item_type_manager_;
public:
    RegistersVectorItem(Das::Database::Device_Item_Type_Manager* mng, QModbusServer* modbus_server, QModbusDataUnit::RegisterType type, const QVector<Das::Device_Item*>& items, QObject* parent = nullptr);
    QVector<Das::Device_Item*> assign(const QVector<Das::Device_Item*>& items);

    Das::Device_Item* at(int i) const;
    QModbusDataUnit::RegisterType type() const;
    QVector<Das::Device_Item*> items() const;
    Das::Database::Device_Item_Type_Manager* manager() const;
    QModbusServer* modbus_server() const;

    QModbusDataUnit modbus_data_unit() const;
};

#endif // REGISTERS_VECTOR_ITEM_H
