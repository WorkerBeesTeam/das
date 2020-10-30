#include "plugin_type.h"

namespace Das {
namespace DB {

Plugin_Type::Plugin_Type(uint32_t id, const QString& name, const QStringList& param_names_device, const QStringList& param_names_device_item) :
    Named_Type(id, name),
    param_names_device_(param_names_device),
    param_names_device_item_(param_names_device_item)
{
}

const QStringList& Plugin_Type::param_names_device() const
{
    return param_names_device_;
}

void Plugin_Type::set_param_names_device(const QStringList& param_names_device)
{
    param_names_device_ = param_names_device;
    param_names_device_.removeAll(QString(""));
}

QVariant Plugin_Type::param_names_device_to_db() const
{
    return param_names_device_.join('|');
}

void Plugin_Type::set_param_names_device_from_db(const QVariant& data)
{
    param_names_device_ = data.toString().split('|');
    param_names_device_.removeAll(QString(""));
}

const QStringList& Plugin_Type::param_names_device_item() const
{
    return param_names_device_item_;
}

void Plugin_Type::set_param_names_device_item(const QStringList& param_names_device_item)
{
    param_names_device_item_ = param_names_device_item;
    param_names_device_item_.removeAll(QString(""));
}

QVariant Plugin_Type::param_names_device_item_to_db() const
{
    return param_names_device_item_.join('|');
}

void Plugin_Type::set_param_names_device_item_from_db(const QVariant& data)
{
    param_names_device_item_ = data.toString().split('|');
    param_names_device_item_.removeAll(QString(""));
}

QDataStream &operator<<(QDataStream &ds, const Plugin_Type &item)
{
    return ds << static_cast<const Named_Type&>(item) << item.param_names_device() << item.param_names_device_item();
}

QDataStream &operator>>(QDataStream &ds, Plugin_Type &item)
{
    return ds >> static_cast<Named_Type&>(item) >> item.param_names_device_ >> item.param_names_device_item_;
}

} // namespace DB
} // namespace Das
