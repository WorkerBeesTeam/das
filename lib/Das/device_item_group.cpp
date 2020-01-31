#include <QDebug>
#include <QMetaMethod>
#include <QMutexLocker>

#include <set>

#include "db/dig_type.h"
#include "section.h"
#include "scheme.h"
#include "device_item_group.h"

namespace Das {

Device_item_Group::Device_item_Group(uint32_t id, const QString& title, uint32_t section_id, uint32_t type_id, uint32_t mode_id) :
    QObject(), Database::Device_Item_Group(id, title, section_id, type_id),
    mode_id_(mode_id), sct_(nullptr), type_(nullptr), param_group_(std::make_shared<Param>(this))
{
    qRegisterMetaType<std::map<uint32_t, QStringList>>("std::map<uint32_t, QStringList>");
    qRegisterMetaType<Das::Param*>("Das::Param*");
}

Device_item_Group::Device_item_Group(Database::Device_Item_Group &&o, uint32_t mode_id) :
    QObject(), Database::Device_Item_Group(std::move(o)),
    mode_id_(mode_id), sct_(nullptr), type_(nullptr), param_group_(std::make_shared<Param>(this))
{
}

Device_item_Group::Device_item_Group(Device_item_Group &&o) :
    QObject(), Database::Device_Item_Group(std::move(o)),
    mode_id_(std::move(o.mode_id_)), sct_(std::move(o.sct_)),
    type_(std::move(o.type_)), param_group_(std::move(o.param_group_))
{
}

Device_item_Group::Device_item_Group(const Device_item_Group &o) :
    QObject(), Database::Device_Item_Group(o),
    mode_id_(o.mode_id_), sct_(o.sct_), type_(o.type_), param_group_(o.param_group_)
{
}

Device_item_Group &Device_item_Group::operator =(Device_item_Group &&o)
{
    Database::Device_Item_Group::operator =(std::move(o));
    mode_id_ = std::move(o.mode_id_);
    sct_ = std::move(o.sct_);
    type_ = std::move(o.type_);
    param_group_ = std::move(o.param_group_);
    return *this;
}

Device_item_Group &Device_item_Group::operator =(const Device_item_Group &o)
{
    Database::Device_Item_Group::operator =(o);
    mode_id_ = o.mode_id_;
    sct_ = o.sct_;
    type_ = o.type_;
    param_group_ = o.param_group_;
    return *this;
}

QString Device_item_Group::name() const
{
    if (title().isEmpty())
    {
        QString name_str = type_ ? type_->title() : QString();
        return name_str.isEmpty() ? QString::number(type_id()) : name_str;
    }
    return title();
}

uint32_t Device_item_Group::mode_id() const { return mode_id_; }
void Device_item_Group::set_mode_id(uint32_t mode_id) { mode_id_ = mode_id; }

const Device_Items &Device_item_Group::items() const { return items_; }
Param *Device_item_Group::params() { return param_group_.get(); }
void Device_item_Group::set_params(std::shared_ptr<Param> param_group) { param_group_ = std::move(param_group); }

Section *Device_item_Group::section() const { return sct_; }
void Device_item_Group::set_section(Section *section)
{
    sct_ = section;
    type_ = sct_->type_managers()->group_type_mng_.get_type(type_id());
}

void Device_item_Group::add_item(Device_Item *item)
{
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it == items_.end())
    {
        items_.push_back( item );
        connect(item, &Device_Item::value_changed, this, &Device_item_Group::value_changed);
        connect(item, &Device_Item::connection_state_changed, this, &Device_item_Group::connection_state_changed);
    }

    if (item->group() != this)
        item->set_group(this);
}

void Device_item_Group::remove_item(Device_Item *item)
{
    if (item)
        item->disconnect(this);

    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end())
        items_.erase(it);
}

void Device_item_Group::finalize()
{
    std::sort(items_.begin(), items_.end(), [](Device_Item* dev1, Device_Item* dev2) -> bool
    {
        const bool dev1IsCtrl = dev1->is_control();
        if (dev1IsCtrl != dev2->is_control())
            return dev1IsCtrl;
        return dev1->type_id() < dev2->type_id();
    });

    if (sct_)
        emit sct_->group_initialized(this);
}

const std::map<uint32_t, QStringList> &Device_item_Group::get_statuses() const { return statuses_; }
void Device_item_Group::set_statuses(const std::map<uint32_t, QStringList> &statuses) { statuses_ = statuses; }

QString Device_item_Group::toString() const
{
    return sct_->name() + ' ' + name();
}

void Device_item_Group::set_mode(uint32_t mode_id, uint32_t user_id)
{
    if (mode_id_ == mode_id)
        return;

    mode_id_ = mode_id;
    emit mode_changed(user_id, mode_id_, id());
}

const std::map<uint32_t, QStringList> &Device_item_Group::statuses() const
{
    return statuses_;
}

bool Device_item_Group::check_status(uint32_t info_id)
{
    return statuses_.find(info_id) != statuses_.cend();
}

void Device_item_Group::add_status(uint32_t info_id, const QStringList &args, uint32_t user_id)
{
    if (statuses_.find(info_id) != statuses_.cend())
        return;
    if (info_id == 0)
    {
        qCWarning(SchemeDetailLog).noquote().nospace() << user_id << '|' << tr("Attempt to set zero status to group:") << ' ' << toString();
        return;
    }
    statuses_.emplace(info_id, args);
    emit status_added(id(), info_id, args, user_id);
}

void Device_item_Group::remove_status(uint32_t info_id, uint32_t user_id)
{
    auto it = statuses_.find(info_id);
    if (it == statuses_.cend())
        return;
    statuses_.erase(it);
    emit status_removed(id(), info_id, user_id);
}

void Device_item_Group::clear_status(uint32_t user_id)
{
    while (statuses_.size())
    {
        emit status_removed(id(), statuses_.begin()->first, user_id);
        statuses_.erase(statuses_.begin());
    }
}

bool Device_item_Group::write_to_control(uint32_t type_id, const QVariant &raw_data, uint32_t mode_id, uint32_t user_id)
{
    if (auto item = item_by_type(type_id))
        return write_to_control(item, raw_data, mode_id, user_id);
    return false;
}

bool Device_item_Group::write_to_control(Device_Item *item, const QVariant &raw_data, uint32_t mode_id, uint32_t user_id)
{
    return item->write(raw_data, mode_id, user_id);
}

Device_Item *Device_item_Group::item_by_type(uint type_id) const
{
    for (Device_Item* item: items_)
        if (item->type_id() == type_id)
            return item;
    return nullptr;
}

QVector<Device_Item *> Device_item_Group::items_by_type(uint type_id) const
{
    QVector<Device_Item *> items;
    for (Device_Item* item: items_)
        if (item->type_id() == type_id)
            items.push_back(item);
    return items;
}

void Device_item_Group::value_changed(uint32_t user_id, const QVariant& old_raw_value)
{
    Device_Item* item = static_cast<Device_Item*>(sender());
    if (!item)
        return;

    emit item_changed(item, user_id, old_raw_value);

    if (item->is_control())
        emit control_changed(item, user_id, old_raw_value);
    else
        emit sensor_changed(item, user_id, old_raw_value);
}

void Device_item_Group::connection_state_changed(bool value)
{
    Device_Item* item = static_cast<Device_Item*>(sender());
    if (!item)
        return;

    emit connection_state_change(item, value);
}

QDataStream &operator<<(QDataStream &ds, Device_item_Group *group)
{
    return ds << *group;
}

} // namespace Das
