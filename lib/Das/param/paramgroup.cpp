#include <QDebug>

#include "Das/device_item_group.h"
#include "paramgroup.h"

namespace Das {

/*static*/ DIG_Param_Type Param::empty_type;

Param::Param(Device_item_Group *owner) :
    Param{ QVariant(), 0, &Param::empty_type, owner }
{
}

Param::Param(uint id, DIG_Param_Type *param, const QString &raw_value, Device_item_Group *owner) :
    Param{ value_from_string(param ? param->value_type() : DIG_Param_Type::VT_UNKNOWN, raw_value), id, param, owner }
{
}

Param::Param(const QVariant &value, uint id, DIG_Param_Type *param, Device_item_Group *owner) :
    QObject(), id_(id), type_(param), group_(owner), value_(value)
{
    if (value_.type() == QVariant::String && param->value_type() != DIG_Param_Type::VT_STRING)
        value_ = value_from_string(param->value_type(), value.toString());
}

Device_item_Group *Param::group() const { return group_; }
void Param::set_group(Device_item_Group *group) { group_ = group; }

/*static*/ QVariant Param::value_from_string(DIG_Param_Type::Value_Type value_type, const QString &raw_value)
{
    switch (value_type) {
    case DIG_Param_Type::VT_INT:       return raw_value.toInt();
    case DIG_Param_Type::VT_BOOL:
    {
        bool is_convert_ok = false;
        int tmp_raw_int = raw_value.toInt(&is_convert_ok);
        if (is_convert_ok)
        {
            return tmp_raw_int ? true : false;
        }
        else
        {
            QString tmp_string = raw_value.toLower();
            return tmp_string.contains("true");
        }
    }
    case DIG_Param_Type::VT_FLOAT:     return QString(raw_value).replace(',', '.').toDouble();
    case DIG_Param_Type::VT_BYTES:     return QByteArray::fromHex(raw_value.toLocal8Bit());
    case DIG_Param_Type::VT_STRING:
    case DIG_Param_Type::VT_TIME:
    case DIG_Param_Type::VT_RANGE:
    case DIG_Param_Type::VT_COMBO:
    default:
        return raw_value;
    }
}

/*static*/ QString Param::value_to_string(DIG_Param_Type::Value_Type value_type, const QVariant &value)
{
    switch (value_type) {
    case DIG_Param_Type::VT_BOOL:
        return QChar(value.toBool() ? '1' : '0');
    case DIG_Param_Type::VT_BYTES:
        return value.toByteArray().toHex().toUpper();
    default:
        return value.toString();
    }
}

void Param::set_value(const QVariant &param_value, uint32_t user_id)
{
    if (value_.type() == param_value.type() && value_ == param_value)
        return;
    if (param_value.type() == QVariant::String && type()->value_type() != DIG_Param_Type::VT_STRING)
    {
        QVariant tmp_value = value_from_string(type()->value_type(), param_value.toString());
        if (tmp_value == value_)
            return;
        value_ = tmp_value;
    }
    else
        value_ = param_value;

    emit value_changed();
    if (group_)
        emit group_->param_changed(this, user_id);
}

void Param::set_value_from_string(const QString &raw_value, uint32_t user_id)
{
    set_value(value_from_string(type()->value_type(), raw_value), user_id);
}

QString Param::value_to_string() const
{
    return value_to_string(type()->value_type(), value_);
}

std::size_t Param::count() const { return childrens_.size(); }

uint Param::id() const { return id_; }
DIG_Param_Type *Param::type() const { return type_; }
const QVariant &Param::value() const { return value_; }

bool Param::add(const DIG_Param &dig_param, const QString& value_string, DIG_Param_Type_Manager* mng)
{
    return add(std::make_shared<Param>(dig_param.id(), mng->get_type(dig_param.param_id()), value_string, group()));
}

bool Param::add(const std::shared_ptr<Param>& elem)
{
    if (!elem || !elem->type_)
        return false;

    bool finded = elem->type_->parent_id == 0;

    if (finded)
    {
        add_child(elem);
    }
    else
    {
        for (std::shared_ptr<Param>& obj: childrens_)
        {
            if (obj->type_->id() == elem->type_->parent_id)
            {
                finded = true;
                obj->add_child(elem);
                break;
            }
            else if (obj->add(elem))
            {
                finded = true;
                break;
            }
        }
    }

    return finded;
}

bool Param::has(const QString &name) const
{
    return get_iterator(name) != childrens_.cend();
}

Param *Param::get(const QString &name) const
{
    auto it = get_iterator(name);
    return it != childrens_.cend() ? it->get() : nullptr;
}

Param *Param::get(uint index) const
{
    return childrens_.size() > index ? childrens_.at(index).get() : nullptr;
}

Param *Param::get_by_id(uint id) const
{
    Param* out_param = nullptr;
    std::find_if(childrens_.cbegin(), childrens_.cend(), [&id, &out_param](const std::shared_ptr<Param>& elem) {
        if (elem->id_ == id)
            out_param = elem.get();
        else
            out_param = elem->get_by_id(id);
        return out_param != nullptr;
    });
    return out_param;
}

Param *Param::get_by_type_id(uint type_id) const
{
    auto it = std::find_if(childrens_.cbegin(), childrens_.cend(), [&type_id](const std::shared_ptr<Param>& elem) {
        return elem->type_->id() == type_id;
    });
    return it != childrens_.cend() ? it->get() : nullptr;
}

std::vector<std::shared_ptr<Param>>::const_iterator Param::get_iterator(const QString &name) const
{
    return std::find_if(childrens_.cbegin(), childrens_.cend(), [&name](const std::shared_ptr<Param>& elem) -> bool {
        return elem->type_ && elem->type_->name() == name;
    });
}

void Param::add_child(const std::shared_ptr<Param> &elem)
{
    childrens_.push_back(elem);
    if (elem->group() != group())
    {
        elem->set_group(group());
    }
}

} // namespace Das
