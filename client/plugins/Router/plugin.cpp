
#include <QDebug>
#include <QSettings>
#include <QFile>

#include <Helpz/zstring.h>
#include <Helpz/zfile.h>
#include <Helpz/settings.h>
#include <Helpz/settingshelper.h>

#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/scheme.h>

#include "wpa_supplicant_config.h"
#include "plugin.h"

namespace Das {
namespace Router {

Q_LOGGING_CATEGORY(RouterLog, "router")

// ----------

namespace Z = Helpz;

Plugin::Plugin() :
    QObject(),
    _params{{nullptr}},
    _dev_items{{nullptr}}
{
}

Plugin::~Plugin()
{
    stop();
}

void Plugin::configure(QSettings *settings)
{
    break_ = false;

    using Helpz::Param;
    conf_ = Helpz::SettingsHelper(
                settings, "Router",
                Param<std::string>{"hostapd", "/etc/hostapd/hostapd.conf"},
                Param<std::string>{"wpa_supplicant", "/etc/wpa_supplicant/wpa_supplicant.conf"}
    ).obj<Config>();

    parse_scheme(settings);

    thread_ = std::thread(&Plugin::run, this);
}

bool Plugin::check(Device* dev)
{
    std::vector<Device_Item*> device_items_disconected;

    for (Device_Item * item: dev->items())
    {
        if (!add_item_check(item))
            device_items_disconected.push_back(item);
    }

    if (!device_items_disconected.empty())
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconected));

    cond_.notify_one();

    return true;
}

void Plugin::stop()
{
    {
        std::lock_guard lock(mutex_);
        break_ = true;
    }
    cond_.notify_one();

    if (thread_.joinable())
        thread_.join();
}
void Plugin::write(std::vector<Write_Cache_Item>& /*items*/) {}

Param* parse_param(Das::Param* param, const QStringList& param_name_list)
{
    if (param_name_list.isEmpty())
        return nullptr;

    for (const QString& param_name: param_name_list)
    {
        if (!param)
            break;
        param = param->get(param_name);
    }
    return param;
}

void Plugin::parse_scheme(QSettings *settings)
{
    auto [ wifi_ap_param_name_prefix, wifi_saved_ap_list_param_name ]
            = Helpz::SettingsHelper(
                settings, "Router",
                Helpz::Param<QString>{"WIFIAPParamNamePrefix", "wifi.ap"},
                Helpz::Param<QString>{"WIFISavedAPListParamName", "wifi.saved"}
    )();

    const QStringList ap_prefix_list = wifi_ap_param_name_prefix.split('.');
    const QStringList wifi_saved_list = wifi_saved_ap_list_param_name.split('.');
    Param*& ap_ssid_param    = _params[PT_AP_SSID];
    Param*& ap_pwd_param     = _params[PT_AP_PWD];
    Param*& ap_channel_param = _params[PT_AP_CHANNEL];
    Param*& wifi_saved_param = _params[PT_WIFI_SAVED_LIST];

    auto item_type_names
            = Helpz::SettingsHelper(
                settings, "Router",
                Helpz::Param<QString>{"NetItemName", "net"},            // DIT_NET = 0
                Helpz::Param<QString>{"CPUTempItemName", "cpu_temp"},   // DIT_CPU_TEMP = 1
                Helpz::Param<QString>{"UsageCPUItemName", "cpu"},       // DIT_CPU = 2
                Helpz::Param<QString>{"UsageRAMItemName", "ram"},       // DIT_RAM = 3
                Helpz::Param<QString>{"UsageDiskSpaceItemName", "disk"} // DIT_DISK = 4
    )();

    auto item_types = std::apply([this](auto&&... x) { return std::array{scheme()->device_item_type_mng_.get_type(std::forward<decltype(x)>(x)) ... }; }, std::move(item_type_names));

    for (Section* sct: scheme()->sections())
    {
        for (Device_item_Group* group: sct->groups())
        {
            if (!ap_channel_param || !ap_pwd_param || !ap_ssid_param)
            {
                Param* const p = parse_param(group->params(), ap_prefix_list);
                if (p)
                {
                    if (!ap_channel_param)
                        ap_channel_param = p->get("channel");
                    if (!ap_pwd_param)
                        ap_pwd_param = p->get("pwd");
                    if (!ap_ssid_param)
                        ap_ssid_param = p->get("ssid");
                }
            }

            if (!wifi_saved_param)
                wifi_saved_param = parse_param(group->params(), wifi_saved_list);

            for (int i = 0; i < Device_Item_Type_Count; ++i)
            {
                Device_Item*& item = _dev_items[i];
                if (!item)
                {
                    DB::Device_Item_Type* type = item_types[i];
                    if (type && type->group_type_id == group->type_id())
                        item = group->item_by_type(type->id());
                }
            }
        }
    }
}

void Plugin::init_params()
{
    if (_params[PT_WIFI_SAVED_LIST])
    {
        WPA_Supplicant_Config wpa_conf(conf_._wpa_supplicant);
        const QString text = QString::fromStdString(wpa_conf.get_str());
        QMetaObject::invokeMethod(_params[PT_WIFI_SAVED_LIST], "set_value", Qt::BlockingQueuedConnection, Q_ARG(QVariant, text));
        connect(_params[PT_WIFI_SAVED_LIST], &Param::value_changed, [this]()
        {
            WPA_Supplicant_Config wpa_conf(conf_._wpa_supplicant);
            wpa_conf.set_str(_params[PT_WIFI_SAVED_LIST]->value().toString().toStdString());
        });
    }

    if (_params[PT_AP_SSID] || _params[PT_AP_PWD] || _params[PT_AP_CHANNEL])
    {
        const std::string text = Z::File::read_all(conf_._hostapd);
        auto init_hostapd_param = [this, &text](const std::string& param, int index)
        {
            if (!_params[index])
                return;

            std::size_t pos = std::string::npos;
            std::string value = Z::Settings::get_param_value(text, param, &pos);
            if (pos != std::string::npos)
            {
                QMetaObject::invokeMethod(_params[index], "set_value", Qt::BlockingQueuedConnection, Q_ARG(QVariant, QString::fromStdString(value)));
                connect(_params[index], &Param::value_changed, [this, param, index]()
                {
                    // TODO: Копить изменения и по таймауту сохранять
                    Z::File file(conf_._hostapd, Z::File::READ_WRITE);
                    if (file.is_opened())
                    {
                        std::string text = file.read_all();
                        Z::Settings::set_param_value(text, param, _params[index]->value().toString().toStdString());
                        file.write(text);
                    }
                    else
                        qWarning(RouterLog) << "Rewrite params failed! Error: " << strerror(errno);
                });
            }
        };

        init_hostapd_param("channel", PT_AP_CHANNEL);
        init_hostapd_param("ssid", PT_AP_SSID);
        init_hostapd_param("wpa_passphrase", PT_AP_PWD);
    }
}

bool Plugin::add_item_check(Device_Item *item)
{
    for (int item_type = 0; item_type < Device_Item_Type_Count; ++item_type)
    {
        if (item == _dev_items[item_type])
        {
            std::lock_guard lock(mutex_);
            data_.insert(static_cast<Device_Item_Type>(item_type));
            return true;
        }
    }

    return false;
}

void Plugin::run()
{
    init_params();

    struct Device_Data_Pack
    {
        std::map<Device_Item*, Device::Data_Item> values_;
        std::vector<Device_Item*> disconnected_vect_;
    };

    std::map<Device*, Device_Data_Pack> devices_data_pack;
    Device::Data_Item data_item{0, 0, {}};

    Device_Item_Type item_type;
    Device_Item* item;

    std::unique_lock lock(mutex_, std::defer_lock);

    while (true)
    {
        lock.lock();
        cond_.wait(lock, [this, &devices_data_pack](){ return break_ || !data_.empty() || !devices_data_pack.empty(); });

        if (break_) break;

        if (!data_.empty())
        {
            item_type = *data_.begin();
            lock.unlock();

            item = _dev_items[item_type];
            if (item)
            {
                Device_Data_Pack& device_data_pack = devices_data_pack[item->device()];
                data_item.raw_data_ = check_item(item_type);
                if (data_item.raw_data_.isValid())
                {
                    data_item.timestamp_msecs_ = DB::Log_Base_Item::current_timestamp();
                    device_data_pack.values_.emplace(item, data_item);
                }
                else
                    device_data_pack.disconnected_vect_.push_back(item);
            }

            std::lock_guard erase_lock(mutex_);
            data_.erase(data_.begin());
        }
        else
        {
            lock.unlock();

            for (const auto& it: devices_data_pack)
            {
                if (!it.second.values_.empty())
                    QMetaObject::invokeMethod(it.first, "set_device_items_values", Qt::QueuedConnection,
                                      QArgument<std::map<Device_Item*, Device::Data_Item>>
                                      ("std::map<Device_Item*, Device::Data_Item>", it.second.values_),
                                      Q_ARG(bool, true));

                if (!it.second.disconnected_vect_.empty())
                    QMetaObject::invokeMethod(it.first, "set_device_items_disconnect", Qt::QueuedConnection,
                                              Q_ARG(std::vector<Device_Item*>, it.second.disconnected_vect_));
            }
            devices_data_pack.clear();
        }
    }
}

QVariant Plugin::check_item(int item_type)
{
    switch (item_type)
    {
    case DIT_NET:       return check_item_net();
    case DIT_CPU_TEMP:  return _sys_info.get_cpu_temp();
    case DIT_CPU:       return _sys_info.get_cpu_load();
    case DIT_RAM:       return _sys_info.get_mem_usage();
    case DIT_DISK:      return _sys_info.get_disk_space_usage();
    default:
        break;
    }
    return {};
}

QVariant Plugin::check_item_net() const
{
    const std::vector<std::string> net_info = _sys_info.get_net_info();
    if (net_info.empty())
        return {};

    std::string text;

    if (!manager()->is_server_connected())
        text = _sys_info.check_internet_connection() ? "Сервер не доступен!" : "Сеть не доступна!";

    for (const std::string& net: net_info)
    {
        if (!text.empty())
            text += '\n';
        text += net;
    }

    return QString::fromStdString(text);
}

} // namespace Router
} // namespace Das
