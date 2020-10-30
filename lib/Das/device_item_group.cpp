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
    QObject(), DB::Device_Item_Group(id, title, section_id, type_id),
    mode_(0, 0, id, mode_id), sct_(nullptr), type_(nullptr), param_group_(std::make_shared<Param>(this))
{
    qRegisterMetaType<std::set<DIG_Status>>("std::set<DIG_Status>");
    qRegisterMetaType<Das::Param*>("Das::Param*");
}

Device_item_Group::Device_item_Group(DB::Device_Item_Group &&o, uint32_t mode_id) :
    QObject(), DB::Device_Item_Group(std::move(o)),
    mode_(0, 0, id(), mode_id), sct_(nullptr), type_(nullptr), param_group_(std::make_shared<Param>(this))
{
}

Device_item_Group::Device_item_Group(Device_item_Group &&o) :
    QObject(), DB::Device_Item_Group(std::move(o)),
    mode_(std::move(o.mode_)), sct_(std::move(o.sct_)),
    type_(std::move(o.type_)), param_group_(std::move(o.param_group_))
{
}

Device_item_Group::Device_item_Group(const Device_item_Group &o) :
    QObject(), DB::Device_Item_Group(o),
    mode_(o.mode_), sct_(o.sct_), type_(o.type_), param_group_(o.param_group_)
{
}

Device_item_Group &Device_item_Group::operator =(Device_item_Group &&o)
{
    DB::Device_Item_Group::operator =(std::move(o));
    mode_ = std::move(o.mode_);
    sct_ = std::move(o.sct_);
    type_ = std::move(o.type_);
    param_group_ = std::move(o.param_group_);
    return *this;
}

Device_item_Group &Device_item_Group::operator =(const Device_item_Group &o)
{
    DB::Device_Item_Group::operator =(o);
    mode_ = o.mode_;
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

uint32_t Device_item_Group::mode_id() const { return mode_.mode_id(); }
const DIG_Mode &Device_item_Group::mode_data() const { return mode_; }

const Device_Items &Device_item_Group::items() const { return items_; }
Param *Device_item_Group::params() { return param_group_.get(); }
const Param *Device_item_Group::params() const { return param_group_.get(); }
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

const std::set<DIG_Status> &Device_item_Group::get_statuses() const { return statuses_; }
void Device_item_Group::set_statuses(const std::set<DIG_Status> &statuses) { statuses_ = statuses; }

QString Device_item_Group::toString() const
{
    return sct_->name() + ' ' + name();
}

void Device_item_Group::set_mode(uint32_t mode_id, uint32_t user_id, qint64 timestamp_msec)
{
    if (mode_.mode_id() == mode_id)
        return;

    mode_.set_mode_id(mode_id);
    mode_.set_user_id(user_id);
    mode_.set_timestamp_msecs(timestamp_msec);

    if (mode_.group_id() != id())
        mode_.set_group_id(id());

    emit mode_changed(user_id, mode_id, id());
}

const std::set<DIG_Status> &Device_item_Group::statuses() const
{
    return statuses_;
}

bool Device_item_Group::check_status(uint32_t status_id) const
{
    for (const DIG_Status& item: statuses_)
        if (item.status_id() == status_id)
            return true;
    return false;
}

void Device_item_Group::add_status(uint32_t status_id, const QStringList &args, uint32_t user_id)
{
    if (check_status(status_id))
        return;

    if (status_id == 0)
    {
        qCWarning(SchemeDetailLog).noquote().nospace() << user_id << '|' << tr("Attempt to set zero status to group:") << ' ' << toString();
        return;
    }

    const DIG_Status status(DB::Log_Base_Item::current_timestamp(), user_id, id(), status_id, args);

    statuses_.emplace(status);
    emit status_changed(status);
}

void Device_item_Group::remove_status(uint32_t status_id, uint32_t user_id)
{
    for (auto it = statuses_.begin(); it != statuses_.end(); ++it)
    {
        if (it->status_id() == status_id)
        {
            DIG_Status status = *it;
            status.set_direction(DIG_Status::SD_DEL);
            status.set_user_id(user_id);

            statuses_.erase(it);
            emit status_changed(status);
            break;
        }
    }
}

void Device_item_Group::clear_status(uint32_t user_id)
{
    while (statuses_.size())
    {
        DIG_Status status{std::move(*statuses_.begin())};
        status.set_direction(DIG_Status::SD_DEL);
        status.set_user_id(user_id);

        statuses_.erase(statuses_.begin());
        emit status_changed(status);
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
