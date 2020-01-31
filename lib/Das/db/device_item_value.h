#ifndef DAS_DEVICE_ITEM_VALUE_H
#define DAS_DEVICE_ITEM_VALUE_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT Device_Item_Value : public Schemed_Model
{
    HELPZ_DB_META(Device_Item_Value, "device_item_value", "sdiv", 5, DB_A(id), DB_A(device_item_id),
                  DB_AT(raw), DB_AT(display), DB_A(scheme_id))
public:
    Device_Item_Value(Device_Item_Value&& other) = default;
    Device_Item_Value(const Device_Item_Value& other) = default;
    Device_Item_Value& operator=(Device_Item_Value&& other) = default;
    Device_Item_Value& operator=(const Device_Item_Value& other) = default;
    Device_Item_Value(uint32_t device_item_id = 0, const QVariant& raw = QVariant(), const QVariant& display = QVariant());

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t device_item_id() const;
    void set_device_item_id(uint32_t device_item_id);

    QVariant raw() const;
    void set_raw(const QVariant& raw);
    QVariant raw_to_db() const;
    void set_raw_from_db(const QVariant& value);

    QVariant display() const;
    void set_display(const QVariant& display);
    QVariant display_to_db() const;
    void set_display_from_db(const QVariant& value);

    static QVariant prepare_value(const QVariant& var);
    static QVariant variant_from_string(const QVariant& var);
private:
    uint32_t id_, device_item_id_;
    QVariant raw_, display_;

    friend QDataStream &operator>>(QDataStream &ds, Device_Item_Value& item);
};

QDataStream &operator>>(QDataStream &ds, Device_Item_Value& item);
QDataStream &operator<<(QDataStream &ds, const Device_Item_Value& item);

} // namespace Database

using Device_Item_Value = Database::Device_Item_Value;

} // namespace Das

Q_DECLARE_METATYPE(Das::Device_Item_Value)

#endif // DAS_DEVICE_ITEM_VALUE_H
