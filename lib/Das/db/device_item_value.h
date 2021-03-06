#ifndef DAS_DEVICE_ITEM_VALUE_H
#define DAS_DEVICE_ITEM_VALUE_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/log/log_base_item.h>

namespace Das {
namespace DB {

#define DEV_ITEM_DB_META(x, y, z) \
    HELPZ_DB_META(x, y, z, DB_A(id), \
              DB_A(timestamp_msecs), DB_AN(user_id), DB_A(item_id), \
              DB_AT(raw_value), DB_AT(value), DB_A(scheme_id))

class DAS_LIBRARY_SHARED_EXPORT Device_Item_Value : public Log_Base_Item
{
    DEV_ITEM_DB_META(Device_Item_Value, "device_item_value", "sdiv")
public:
    Device_Item_Value(Device_Item_Value&& other) = default;
    Device_Item_Value(const Device_Item_Value& other) = default;
    Device_Item_Value& operator=(Device_Item_Value&& other) = default;
    Device_Item_Value& operator=(const Device_Item_Value& other) = default;
    Device_Item_Value(qint64 timestamp_msecs = 0, uint32_t user_id = 0, uint32_t device_item_id = 0,
                      const QVariant& raw = QVariant(),
                      const QVariant& display = QVariant(),
                      bool flag = false);

    uint32_t item_id() const;
    void set_item_id(uint32_t device_item_id);

    const QVariant& raw_value() const;
    void set_raw_value(const QVariant& raw);
    QVariant raw_value_to_db() const;
    void set_raw_value_from_db(const QVariant& value);

    const QVariant& value() const;
    void set_value(const QVariant& display);
    QVariant value_to_db() const;
    void set_value_from_db(const QVariant& value);

    static QVariant prepare_value(const QVariant& var);
    static QVariant variant_from_string(const QVariant& var);

    bool is_big_value() const;
    static bool is_big_value(const QVariant& val);
private:
    uint32_t item_id_;
    QVariant raw_value_, value_;

    friend QDataStream &operator>>(QDataStream &ds, Device_Item_Value& item);
};

QDataStream &operator>>(QDataStream &ds, Device_Item_Value& item);
QDataStream &operator<<(QDataStream &ds, const Device_Item_Value& item);

} // namespace DB

using Device_Item_Value = DB::Device_Item_Value;

} // namespace Das

Q_DECLARE_METATYPE(Das::Device_Item_Value)

#endif // DAS_DEVICE_ITEM_VALUE_H
