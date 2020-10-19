#include <QDebug>

#include <QMetaMethod>

#include <numeric>

#include "section.h"

namespace Das
{

Section::Section(uint32_t id, const QString& name, uint32_t day_start_secs, uint32_t day_end_secs) :
    QObject(), DB::Named_Type(id, name),
    day_(day_start_secs, day_end_secs), type_mng_(nullptr)
{
    qRegisterMetaType<Device_Item*>("Device_Item*");
}

Section::Section(Section &&o) :
    QObject(), DB::Named_Type(std::move(o)),
    day_(std::move(o.day_)), type_mng_(std::move(o.type_mng_))
{
}

Section::Section(const Section &o) :
    QObject(), DB::Named_Type(o),
    day_(o.day_), type_mng_(o.type_mng_)
{
}

Section &Section::operator =(Section &&o)
{
    DB::Named_Type::operator =(std::move(o));
    day_ = std::move(o.day_);
    type_mng_ = std::move(o.type_mng_);
    return *this;
}

Section &Section::operator =(const Section &o)
{
    DB::Named_Type::operator =(o);
    day_ = o.day_;
    type_mng_ = o.type_mng_;
    return *this;
}

Section::~Section()
{
    item_delete_later(groups_);
}

//Section::Section(const Prt::Section &sct, Type_Managers *typeMng, QObject *parent) :
//    QObject(parent), Prt::Section(sct), typeMng_(typeMng)
//{
//    qRegisterMetaType<Device_Item*>("Device_Item*");
//}

TimeRange* Section::day_time() { return &day_; }
const TimeRange *Section::day_time() const { return &day_; }

Device_item_Group *Section::add_group(DB::Device_Item_Group&& grp, uint32_t mode_id)
{
    Device_item_Group* group = new Device_item_Group( std::move(grp), mode_id );
    group->set_section(this);
    connect(group, &Device_item_Group::item_changed, this, &Section::item_changed);
    connect(group, &Device_item_Group::control_changed, this, &Section::control_changed);
    connect(group, &Device_item_Group::mode_changed, this, &Section::mode_changed);
    connect(group, &Device_item_Group::control_state_changed, this, &Section::control_state_changed);

    groups_.push_back(group);
    return group;
}

void Section::write_to_control(uint type, const QVariant &data, uint32_t mode, uint32_t user_id)
{
    if (auto group = group_by_dev_Device_item_type(type))
        group->write_to_control(type, data, mode, user_id);
}

Type_Managers *Section::type_managers() const { return type_mng_; }
void Section::set_type_managers(Type_Managers *type_mng) { type_mng_ = type_mng; }

const QVector<Device_item_Group *> &Section::groups() const { return groups_; }

Device_Items Section::items() const
{
    Device_Items items;
    for (Device_item_Group* group: groups_)
        for (const auto& item: group->items())
            items.push_back(item);

    std::sort(items.begin(), items.end(), [](Device_Item* item, Device_Item* item2) -> bool
    {
        if (item->is_control() != item2->is_control())
            return item->is_control();

        return item->type_id() < item2->type_id();
    });

    return items;
}

QString Section::toString() const
{
    return name().isEmpty() ? QString::number(id()) : name();
}

QVector<Device_item_Group*> Section::groups_by_dev_Device_item_type(uint device_item_type_id) const
{
    return groups_by_type(type_mng_->device_item_type_mng_.group_type_id(device_item_type_id));
}

Device_item_Group*Section::group_by_dev_Device_item_type(uint device_item_type_id) const
{
    return group_by_type(type_mng_->device_item_type_mng_.group_type_id(device_item_type_id));
}

Device_item_Group *Section::group_by_type(uint g_type) const
{
    if (g_type)
        for (Device_item_Group* group: groups_)
            if (group->type_id() == g_type)
                return group;
    return nullptr;
}

QVector<Device_item_Group*> Section::groups_by_type(uint g_type) const
{
    QVector<Device_item_Group*> groups;
    if (g_type)
        for (Device_item_Group* group: groups_)
            if (group->type_id() == g_type)
                groups.push_back(group);
    return groups;
}

Device_item_Group *Section::group_by_id(uint g_id) const
{
    if (g_id)
        for (Device_item_Group* group: groups_)
            if (group->id() == g_id)
                return group;
    return nullptr;
}

QVector<Device_Item*> Section::items_by_type(uint device_item_type_id, uint32_t group_type_id) const
{
    QVector<Device_item_Group*> groups;
    if (group_type_id)
        groups = groups_by_type(group_type_id);
    else
        groups = groups_by_dev_Device_item_type(device_item_type_id);

    QVector<Device_Item*> dev_items;
    for (Device_item_Group* group: groups)
        dev_items += group->items_by_type(device_item_type_id);
    return dev_items;
}

Device_Item *Section::item_by_type(uint device_item_type_id, uint32_t group_type_id) const
{
    Device_item_Group* group;
    if (group_type_id)
        group = group_by_type(group_type_id);
    else
        group = group_by_dev_Device_item_type(device_item_type_id);

    if (group)
        return group->item_by_type(device_item_type_id);
    return nullptr;
}

QDataStream &operator>>(QDataStream &ds, Section &sct)
{
    return ds >> static_cast<DB::Named_Type&>(sct) >> sct.day_;
}

QDataStream &operator<<(QDataStream &ds, const Section &sct)
{
    return ds << static_cast<const DB::Named_Type&>(sct) << *sct.day_time();
}

QDataStream &operator<<(QDataStream &ds, Section *sct)
{
    return ds << *sct;
}

} // namespace Das
