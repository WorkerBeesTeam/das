#ifndef DAS_GATT_PLUGIN_H
#define DAS_GATT_PLUGIN_H

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <queue>
#include <set>
#include <unordered_map>

#include <QTimer>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checker_interface.h>
#include <Das/param/paramgroup.h>

#include "gatt_characteristic_receiver.h"

typedef QMap<QString, QVariantMap> InterfaceList;
typedef QMap<QDBusObjectPath, InterfaceList> ManagedObjectList;
Q_DECLARE_METATYPE(InterfaceList)

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(GattLog)

class Gatt_Notification_Listner;

class DAS_PLUGIN_SHARED_EXPORT Gatt_Plugin final : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)

public:
    Gatt_Plugin();
    ~Gatt_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    void start() override;
    bool check(Device *dev) override;
    void stop() override;
    void write(Device* dev, std::vector<Write_Cache_Item>& items) override;
public slots:
    void print_list_devices();
    void find_devices(const QStringList& accepted_services, const QStringList& accepted_characteristics = {});

private slots:
    void props_changed(const QString& iface, const QVariantMap& changed, const QStringList& invalidated);
    void interfaces_added(const QDBusObjectPath& object_path, const InterfaceList& interfaces);
    void interfaces_removed(const QDBusObjectPath& object_path, const QStringList& interfaces);
    void receive_timeout(Device_Item* item);

private:

    using clock = std::chrono::system_clock;

    struct Gatt_Device_Info
    {
        bool is_connected() const
        {
            return _last_connect_time.time_since_epoch() == clock::duration::max();
        }

        bool is_can_connection() const
        {
            return !is_connected()
                    && (_last_connect_time.time_since_epoch() == clock::duration::zero()    //    if no one connection attempt (init)
                        || (clock::now() - _last_connect_time) >= std::chrono::seconds(15));// or if is last fail connection timeout
        }

        Param* _address_param = nullptr;
        QString _address;
        QString _service;
        QString _dev_path;
        std::set<QString> _excluded_devs;
        std::unique_ptr<QDBusInterface> _dev_iface;
        std::unique_ptr<QDBusInterface> _prop_iface;
        std::vector<std::shared_ptr<Gatt::Characteristic_Receiver>> _items_chars;
        clock::time_point _last_connect_time;
    };

    ManagedObjectList get_managed_objects();

    bool start_discovery();
    void restart();

    void init_device(Device* dev);
    void toggle_adapter_power(bool state);

    QString get_address(const InterfaceList& interfaces, QStringList* service_uuids = nullptr);
    QString get_address(const Gatt_Device_Info& info);
    std::map<Device*, Gatt_Device_Info>::iterator get_next_disconnected();

    void connect_next2();
    void process_connected();
    void abort_current_and_connect_next(Gatt_Device_Info* info = nullptr, const QString* address = nullptr);
    bool start_receive(std::shared_ptr<Gatt::Characteristic_Receiver> receiver, const QString& path);

    struct Config
    {
        QString _adapter;
        std::chrono::seconds _scan_timeout;
        std::chrono::seconds _connect_timeout;
        std::chrono::seconds _read_timeout;
    };

    bool _break;
    bool _last_is_ok = true;
    Config _conf;

    QDBusConnection _conn;
    std::unique_ptr<QDBusInterface> _obj_mng;
    std::unique_ptr<QDBusInterface> _adapter;
    std::unique_ptr<QDBusInterface> _prop_iface;

    std::set<QString> _excluded_devs;
    std::map<Device*, Gatt_Device_Info> _devs;

    QTimer _start_timer;

    QTimer _connect_timer;
    Device* _connecting_dev;
    clock::time_point _connect_start_time;
};

} // namespace Das

#endif // DAS_GATTPLUGIN_H
