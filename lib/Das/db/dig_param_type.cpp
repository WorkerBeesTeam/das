#include "Das/param/paramgroup.h"
#include "dig_param_type.h"

namespace Das {
namespace Database {

DIG_Param_Type::DIG_Param_Type(uint id, const QString &name, const QString &title, QString description, Value_Type value_type,
                     uint group_type_id, uint parent_id) :
    QObject(), Titled_Type(id, name, title),
    group_type_id(group_type_id), parent_id(parent_id), value_type_(value_type), description_(description)
{
}

DIG_Param_Type::DIG_Param_Type(const DIG_Param_Type &o) :
    QObject(), Titled_Type{o},
    group_type_id(o.group_type_id), parent_id(o.parent_id), value_type_(o.value_type_), description_(o.description_) {}

DIG_Param_Type::DIG_Param_Type(DIG_Param_Type &&o) :
    QObject(), Titled_Type{std::move(o)},
    group_type_id(std::move(o.group_type_id)), parent_id(std::move(o.parent_id)),
    value_type_(std::move(o.value_type_)), description_(std::move(o.description_)) {}

DIG_Param_Type &DIG_Param_Type::operator=(DIG_Param_Type &&o)
{
    Titled_Type::operator=(std::move(o));
    group_type_id = std::move(o.group_type_id);
    parent_id = std::move(o.parent_id);
    description_ = std::move(o.description_);
    value_type_ = std::move(o.value_type_);
    return *this;
}

DIG_Param_Type &DIG_Param_Type::operator=(const DIG_Param_Type &o)
{
    Titled_Type::operator=(o);
    group_type_id = o.group_type_id;
    parent_id = o.parent_id;
    description_ = o.description_;
    value_type_ = o.value_type_;
    return *this;
}

QString DIG_Param_Type::description() const { return description_; }
void DIG_Param_Type::set_description(const QString &description) { description_ = description; }

DIG_Param_Type::Value_Type DIG_Param_Type::value_type() const { return value_type_; }
void DIG_Param_Type::set_value_type(const Value_Type &value_type) { value_type_ = value_type; }

QDataStream &operator<<(QDataStream &ds, const DIG_Param_Type &param)
{
    const Titled_Type& base = param;
    return ds << base << param.description_ << quint8(param.value_type_) << param.group_type_id << param.parent_id;
}

QDataStream &operator>>(QDataStream &ds, DIG_Param_Type &param)
{
    Titled_Type& base = param;
    return ds >> base >> param.description_ >> reinterpret_cast<quint8&>(param.value_type_) >> param.group_type_id >> param.parent_id;
}

} // namespace Database
} // namespace Das
