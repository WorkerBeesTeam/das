#ifndef DAS_ROUTER_PLUGIN_H
#define DAS_ROUTER_PLUGIN_H

#include <memory>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checker_interface.h>
#include <Das/param/paramgroup.h>

#include "system_informator.h"

namespace Das {
namespace Router {

Q_DECLARE_LOGGING_CATEGORY(RouterLog)

struct Config
{
    std::string _hostapd;
    std::string _wpa_supplicant;
};

class DAS_PLUGIN_SHARED_EXPORT Plugin : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)

public:
    Plugin();
    ~Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;

private:
    void parse_scheme(QSettings* settings);

    void init_params();

    bool add_item_check(Device_Item* item);
    void run();
    QVariant check_item(int item_type);
    QVariant check_item_net() const;

    enum Param_Types {
        PT_AP_SSID,
        PT_AP_PWD,
        PT_AP_CHANNEL,
        PT_WIFI_SAVED_LIST,

        Param_Type_Count
    };

    Param* _params[Param_Type_Count];

    enum Device_Item_Type {
        DIT_NET,
        DIT_CPU_TEMP,
        DIT_CPU,
        DIT_RAM,
        DIT_DISK,

        Device_Item_Type_Count
    };

    Device_Item* _dev_items[Device_Item_Type_Count];

    Config conf_;

    System_Informator _sys_info;

    std::thread thread_;
    std::mutex mutex_;
    bool break_;
    std::condition_variable cond_;

    std::set<Device_Item_Type> data_;
};

} // namespace Router
} // namespace Das

#endif // DAS_ROUTER_PLUGIN_H
