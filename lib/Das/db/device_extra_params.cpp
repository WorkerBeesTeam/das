#include <QJsonDocument>
#include <QJsonArray>

#include "device_extra_params.h"

namespace Das {
namespace Database {

Device_Extra_Params::Device_Extra_Params(const QVariantMap& extra) :
    extra_(extra)
{
}

Device_Extra_Params::Device_Extra_Params(const QVariantList& extra_values)
{
    set_param_value_list(extra_values);
}

const QVariantMap& Device_Extra_Params::params() const { return extra_; }

QVariant Device_Extra_Params::param(const QString& name) const
{
    auto it = extra_.find(name);
    return it == extra_.cend() ? QVariant() : it.value();
}

void Device_Extra_Params::set_param(const QString &name, const QVariant &value)
{
    extra_[name] = value;
}

QVariant Device_Extra_Params::extra_to_db() const
{
    QJsonArray extra = QJsonArray::fromVariantList(params_to_value_list());
    if (extra.isEmpty())
        return {};
    return QJsonDocument(extra).toJson(QJsonDocument::Compact);
}

void Device_Extra_Params::set_extra_from_db(const QVariant& value)
{
    set_param_value_list(QJsonDocument::fromJson(value.toByteArray()).array().toVariantList());
}

void Device_Extra_Params::set_param_name_list(const QStringList& names)
{
    QVariantMap new_extra;
    auto extra_it = extra_.cbegin();
    auto name_it = names.cbegin();
    QString key;

    for (; extra_it != extra_.cend(); ++extra_it)
    {
        if (name_it != names.cend())
        {
            key = *name_it;
            ++name_it;
        }
        else
            key = extra_it.key();
        new_extra.insert(key, *extra_it);
    }

    extra_ = std::move(new_extra);
}

void Device_Extra_Params::set_param_value_list(const QVariantList& values)
{
    auto extra_it = extra_.begin();
    auto value_it = values.cbegin();
    for (int i = 0; value_it != values.cend(); ++i, ++extra_it, ++value_it)
    {
        if (extra_it == extra_.end())
        {
            extra_it = extra_.insert(extra_it, QString::number(i), *value_it);
        }
        else
        {
            *extra_it = *value_it;
        }
    }

    while (extra_it != extra_.end())
    {
        extra_it = extra_.erase(extra_it);
    }
}

QVariantList Device_Extra_Params::params_to_value_list() const
{
    return extra_.values();
}

QDataStream& operator>>(QDataStream& ds, Device_Extra_Params& item)
{
    QVariantList values;
    ds >> values;
    item.set_param_value_list(values);
    return ds;
}

QDataStream& operator<<(QDataStream& ds, const Device_Extra_Params& item)
{
    return ds << item.params_to_value_list();
}

} // namespace Database
} // namespace Das
