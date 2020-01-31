#ifndef DAS_DEVICE_H
#define DAS_DEVICE_H

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/device_item.h>

namespace Das {
namespace Database {
struct Plugin_Type;
struct Device_Item_Type_Manager;
}
class Scheme;

class DAS_LIBRARY_SHARED_EXPORT Device : public QObject, public Database::Base_Type, public Database::Device_Extra_Params
{
    Q_OBJECT
    Q_PROPERTY(uint32_t id READ id WRITE set_id)
    Q_PROPERTY(QString name READ name WRITE set_name)
    Q_PROPERTY(uint32_t plugin_id READ plugin_id)
    Q_PROPERTY(QString checker_name READ checker_name)
    Q_PROPERTY(QObject* checker READ checker)
    Q_PROPERTY(Device_Items items READ items)

    HELPZ_DB_META(Device, "device", "d", 6, DB_A(id), DB_A(name), DB_AT(extra), DB_AN(plugin_id), DB_A(check_interval), DB_A(scheme_id))
public:
    Device(uint32_t id = 0, const QString& name = {}, const QVariantMap& extra = {}, uint32_t plugin_id = 0, uint32_t check_interval = 1500);
    Device(Device&& o);
    Device(const Device& o);
    Device& operator =(Device&& o);
    Device& operator =(const Device& o);
    ~Device();

    uint32_t plugin_id() const;
    void set_plugin_id(uint32_t checker_id);

    uint32_t check_interval() const;
    void set_check_interval(uint32_t check_interval);

    QString checker_name() const;

    QObject* checker();

    Database::Device_Item_Type_Manager *type_mng() const;
    Database::Plugin_Type *checker_type() const;

    const QVector< Device_Item* >& items() const;

    void add_item(Device_Item* item);
    Device_Item* create_item(Device_Item&& device_item);

    void set_scheme(Scheme* scheme);

signals:
    void changed();
public:
    QString toString() const;
public slots:
    void set_device_items_values(const std::map<Das::Device_Item*, QVariant>& device_items_values, bool is_connection_force = false);
    void set_device_items_disconnect(const std::vector<Device_Item*>& device_items);
private:
    uint32_t plugin_id_, check_interval_;
    QVector< Device_Item* > items_;

    Database::Device_Item_Type_Manager* type_mng_;
    Database::Plugin_Type* checker_type_;

    friend QDataStream &operator>>(QDataStream& ds, Device& dev);
};

QDataStream &operator>>(QDataStream& ds, Device& dev);
QDataStream &operator<<(QDataStream& ds, const Device& dev);
QDataStream &operator<<(QDataStream& ds, Device* dev);

} // namespace Das

#endif // DEVICE_H
