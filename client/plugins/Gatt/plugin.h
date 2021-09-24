#ifndef DAS_GATT_PLUGIN_H
#define DAS_GATT_PLUGIN_H

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <unordered_map>

#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checker_interface.h>

#include "gatt_common.h"

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
    bool check(Device *dev) override;
    void stop() override;
    void write(Device* dev, std::vector<Write_Cache_Item>& items) override;
public slots:
    void print_list_devices();
    void find_devices(const QStringList& accepted_services, const QStringList& accepted_characteristics = {});

private:

    struct Config
    {
        std::chrono::seconds _scan_timeout;
        std::chrono::seconds _read_timeout;
    };

    struct Data_Item
    {
        Device* _dev;
        QByteArray _dev_address;

        std::map<Device_Item*, uuid_t> _items;
        std::vector<uuid_t> _characteristics;

        bool operator==(Device* dev) const { return _dev == dev; }
    };

    bool add(Data_Item &data, Device_Item* item);
    void run();
    void read_data_item(const Data_Item& info);

    bool _break;
    bool _last_is_ok = true;
    Config _conf;

    std::thread _thread;
    std::mutex _mutex, _adapter_mutex;
    std::condition_variable _cond;

    std::deque<Data_Item> _data;

    std::shared_ptr<Gatt_Notification_Listner> _notify_listner_ptr;
};

} // namespace Das

#endif // DAS_GATTPLUGIN_H
