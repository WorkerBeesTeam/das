#include <QJsonDocument>

#include "scheme.h"
#include "device_item.h"
#include "device.h"

namespace Das {

Device::Device(uint32_t id, const QString& name, const QVariantMap &extra, uint32_t plugin_id, uint32_t check_interval) :
    QObject(), DB::Named_Type(id, name), DB::Device_Extra_Params(extra),
    plugin_id_(plugin_id), check_interval_(check_interval), type_mng_(nullptr), checker_type_(nullptr)
{
    qRegisterMetaType<std::map<Device_Item*, Device::Data_Item>>("std::map<Device_Item*, Device::Data_Item>");
    qRegisterMetaType<std::vector<Device_Item*>>("std::vector<Device_Item*>");
}

Device::Device(Device &&o) :
    QObject(), DB::Named_Type(std::move(o)), DB::Device_Extra_Params(std::move(o)),
    plugin_id_(std::move(o.plugin_id_)), check_interval_(std::move(o.check_interval_)),
    items_(std::move(o.items_)), type_mng_(std::move(o.type_mng_)), checker_type_(std::move(o.checker_type_))
{
}

Device::Device(const Device &o) :
    QObject(), DB::Named_Type(o), DB::Device_Extra_Params(o),
    plugin_id_(o.plugin_id_), check_interval_(o.check_interval_),
    items_(o.items_), type_mng_(o.type_mng_), checker_type_(o.checker_type_)
{
}

Device &Device::operator =(Device &&o)
{
    DB::Named_Type::operator =(std::move(o));
    DB::Device_Extra_Params::operator =(std::move(o));
    plugin_id_ = std::move(o.plugin_id_);
    check_interval_ = std::move(o.check_interval_);
    items_ = std::move(o.items_);
    type_mng_ = std::move(o.type_mng_);
    checker_type_ = std::move(o.checker_type_);
    return *this;
}

Device &Device::operator =(const Device &o)
{
    DB::Named_Type::operator =(o);
    Device_Extra_Params::operator =(o);
    plugin_id_ = o.plugin_id_;
    check_interval_ = o.check_interval_;
    items_ = o.items_;
    type_mng_ = o.type_mng_;
    checker_type_ = o.checker_type_;
    return *this;
}

Device::~Device()
{
    item_delete_later(items_);
}

uint32_t Device::plugin_id() const { return plugin_id_; }
void Device::set_plugin_id(uint32_t plugin_id) { plugin_id_ = plugin_id; }

QString Device::checker_name() const
{
    return checker_type_ ? checker_type_->name() : QString();
}

QObject* Device::checker()
{
    return checker_type_ && checker_type_->loader ? checker_type_->loader->instance() : nullptr;
}

Device_Item_Type_Manager *Device::type_mng() const { return type_mng_; }
Plugin_Type *Device::checker_type() const { return checker_type_; }

const QVector<Device_Item *> &Device::items() const { return items_; }

void Device::add_item(Device_Item *item) { items_.push_back(item); }

Device_Item *Device::create_item(Device_Item&& device_item)
{
    Device_Item *item = new Device_Item{ std::move(device_item) };
    item->set_device(this);

    if (checker_type_)
        item->set_param_name_list(checker_type_->param_names_device_item());

    items_.push_back(item);
    return item;
}

void Device::set_scheme(Scheme *scheme)
{
    setParent(scheme);

    type_mng_ = &scheme->device_item_type_mng_;
    checker_type_ = scheme->plugin_type_mng_->get_type(plugin_id_);
    if (checker_type_)
    {
        if (!checker_type_->need_it)
            checker_type_->need_it = true;
        set_param_name_list(checker_type_->param_names_device());
    }
}

QString Device::toString() const
{
    return "Device: " + name();
}

void Device::set_device_items_values(const std::map<Device_Item*, Data_Item> &device_items_values, bool is_connection_force)
{
    QVariant old_value;
    std::map<Device_Item*, Data_Item> changed_items;
    std::vector<Device_Item*> connection_change_items;

    for (const std::pair<Device_Item*, Data_Item>& it : device_items_values)
    {
        old_value = it.first->raw_value();

        if (it.first->set_raw_value(it.second.raw_data_, false, it.second.user_id_, true, it.second.timestamp_msecs_))
        {
            changed_items.emplace(it.first, Data_Item{it.second.user_id_, 0, old_value});
        }

        if (is_connection_force && it.second.raw_data_.isValid() && !it.first->is_connected())
        {
            it.first->set_connection_state(true, true);
            connection_change_items.emplace_back(it.first);
        }
    }

    for (const auto& it : connection_change_items)
    {
        emit it->connection_state_changed(it->is_connected());
    }

    for (const std::pair<Device_Item*, Data_Item>& it : changed_items)
    {
        emit it.first->value_changed(it.second.user_id_, it.second.raw_data_);
    }

    if (!changed_items.empty())
        emit changed();
}

void Device::set_device_items_disconnect(const std::vector<Device_Item*>& device_items)
{
    std::vector<Device_Item*> connection_change_items;
    for (const auto& it : device_items)
    {
        if (it->set_connection_state(false, true))
        {
            connection_change_items.emplace_back(it);
        }
    }

    for (const auto& it : connection_change_items)
    {
        emit it->connection_state_changed(it->is_connected());
    }
}

uint32_t Device::check_interval() const
{
    return check_interval_;
}

void Device::set_check_interval(uint32_t check_interval)
{
    check_interval_ = check_interval;
}

QDataStream &operator>>(QDataStream &ds, Device &dev)
{
    return ds >> static_cast<DB::Named_Type&>(dev) >> static_cast<DB::Device_Extra_Params&>(dev) >> dev.plugin_id_ >> dev.check_interval_;
}

QDataStream &operator<<(QDataStream &ds, const Device &dev)
{
    return ds << static_cast<const DB::Named_Type&>(dev) << static_cast<const DB::Device_Extra_Params&>(dev) << dev.plugin_id() << dev.check_interval();
}

QDataStream &operator<<(QDataStream &ds, Device *dev)
{
    return ds << *dev;
}

} // namespace Das
