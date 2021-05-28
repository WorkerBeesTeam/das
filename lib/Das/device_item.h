#ifndef DAS_DEVICEITEM_H
#define DAS_DEVICEITEM_H

#include <mutex>

#include <QObject>
#include <QVector>

#include <Helpz/db_meta.h>

#include "daslib_global.h"

#include <Das/db/device_item.h>
#include <Das/db/device_item_value.h>

namespace Das {

class Device;
typedef Device* DevicePtr;
typedef QVector< DevicePtr > Devices;

class Device_item_Group;

class DAS_LIBRARY_SHARED_EXPORT Device_Item : public QObject, public DB::Device_Item
{
    Q_OBJECT
    Q_PROPERTY(uint32_t id READ id WRITE set_id)
    Q_PROPERTY(QString name READ get_name)
    Q_PROPERTY(QString title READ display_name)
    Q_PROPERTY(uint32_t type READ type_id WRITE set_type_id)
    Q_PROPERTY(uint32_t parent_id READ parent_id)
    Q_PROPERTY(uint32_t device_id READ device_id)
    Q_PROPERTY(uint32_t group_id READ group_id)
    Q_PROPERTY(QVariant raw_value READ raw_value WRITE set_raw_value)
    Q_PROPERTY(QVariant value READ value WRITE set_raw_value)
    Q_PROPERTY(uint8_t register_type READ register_type)
    Q_PROPERTY(Device* device READ device)
    Q_PROPERTY(Device_Item* parent READ parent)
    Q_PROPERTY(Device_item_Group* group READ group)
    Q_PROPERTY(bool is_connected READ is_connected)
    Q_PROPERTY(qint64 value_time READ value_time)
    Q_PROPERTY(uint32_t value_user_id READ value_user_id)
public:
    Device_Item(uint32_t id = 0, const QString& name = {}, uint32_t type_id = 0, const QVariantList& extra_values = {},
               uint32_t parent_id = 0, uint32_t device_id = 0, uint32_t group_id = 0);
    Device_Item(Device_Item&& o);
    Device_Item(const Device_Item& o);
    Device_Item& operator =(Device_Item&& o);
    Device_Item& operator =(const Device_Item& o);

    Q_INVOKABLE QString get_name() const;
    Q_INVOKABLE QString get_title() const;
    Q_INVOKABLE QString display_name() const;
    Q_INVOKABLE QString default_name() const;

    Device_Item* parent() const;
    void set_parent(Device_Item* item);

    Device_item_Group *group() const;
    void set_group(Device_item_Group* group);

    Device *device() const;
    void set_device(Device* device);

    uint8_t register_type() const;

    qint64 value_time() const;
    uint32_t value_user_id() const;

    const DB::Device_Item_Value& data() const;

    QVariant value() const;
    void set_display_value(const QVariant& val); // TODO: DELETE IT

//    bool setData(const Prt::ItemValue& raw, const Prt::ItemValue& val);
    QVariant raw_value() const;
    Q_INVOKABLE QVariant get_raw_value() const;

    static bool is_control(uint register_type);

    Q_INVOKABLE QVector<Device_Item *> childs() const;

signals:
    void connection_state_changed(bool state);
    void value_changed(uint32_t user_id, const QVariant& old_raw_value);
    bool is_can_change(const QVariant& display_value, uint32_t user_id = 0);
    bool clarify_connection_state(bool state);
    QVariant raw_to_display(const QVariant& val);
    QVariant display_to_raw(const QVariant& val);
public slots:
    QString toString(); // Use from QtScript

    bool is_connected() const;
    bool set_connection_state(bool value, bool silent = false);

    bool is_control() const;

    bool write(const QVariant& display_value, uint32_t mode_id = 0, uint32_t user_id = 0);
    bool set_raw_value(const QVariant &raw_data, bool force = false, uint32_t user_id = 0, bool silent = false, qint64 value_time = DB::Log_Base_Item::current_timestamp());
    bool set_data(const DB::Device_Item_Value& data);
    bool set_data(const QVariant& raw, const QVariant& val, uint32_t user_id = 0, qint64 value_time = DB::Log_Base_Item::current_timestamp());
    Device_Item* child(int index) const;

private:
    bool connection_state_real() const;
    void set_connection_state_real(bool state);

    Device* device_;
    Device_item_Group* group_;
    Device_Item* parent_;

    mutable std::recursive_mutex mutex_; // Сейчас в программе есть вызовы функции raw_value из других потоков, поэтому пока добавим mutex.

    DB::Device_Item_Value data_;

    QVector< Device_Item* > childs_;
};

QDataStream &operator<<(QDataStream& ds, Device_Item* item);

typedef QVector< Device_Item* > Device_Items;

} // namespace Das

Q_DECLARE_METATYPE(Das::Device_Items)

#endif // DAS_DEVICEITEM_H
