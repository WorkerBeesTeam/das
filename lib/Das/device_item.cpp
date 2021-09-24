#include <QDebug>
#include <QMetaMethod>

#include "db/device_item_type.h"
#include "scheme.h"
#include "device_item_group.h"
#include "device.h"
#include "device_item.h"

namespace Das {

Device_Item::Device_Item(uint32_t id, const QString& name, uint32_t type_id, const QVariantList& extra_values,
                       uint32_t parent_id, uint32_t device_id, uint32_t group_id) :
    QObject(), DB::Device_Item(id, name, type_id, extra_values, parent_id, device_id, group_id),
    device_(nullptr), group_(nullptr), parent_(nullptr)
{
    set_connection_state(true, true);
}

Device_Item::Device_Item(Device_Item &&o) :
    QObject(), DB::Device_Item(std::move(o)),
    device_(std::move(o.device_)), group_(std::move(o.group_)), parent_(std::move(o.parent_)),
    data_(std::move(o.data_)), childs_(std::move(o.childs_))
{
}

Device_Item::Device_Item(const Device_Item &o) :
    QObject(), DB::Device_Item(o),
    device_(o.device_), group_(o.group_), parent_(o.parent_),
    data_(o.data_), childs_(o.childs_)
{
}

Device_Item &Device_Item::operator =(Device_Item &&o)
{
    DB::Device_Item::operator =(std::move(o));
    device_ = std::move(o.device_);
    group_ = std::move(o.group_);
    parent_ = std::move(o.parent_);
    data_ = std::move(o.data_);
    childs_ = std::move(o.childs_);
    return *this;
}

QString Device_Item::get_name() const
{
    return device_ ? device_->type_mng()->name(type_id()) : QString{};
}

QString Device_Item::get_title() const
{
    return device_ ? device_->type_mng()->title(type_id()) : QString{};
}

Device_Item &Device_Item::operator =(const Device_Item &o)
{
    DB::Device_Item::operator =(o);
    device_ = o.device_;
    group_ = o.group_;
    parent_ = o.parent_;
    data_ = o.data_;
    childs_ = o.childs_;
    return *this;
}

QString Device_Item::display_name() const
{
    return name().isEmpty() ? default_name() : name();
}

QString Device_Item::default_name() const { return device_ ? device_->type_mng()->title(type_id()) : QString{}; }

Device_Item* Device_Item::parent() const { return parent_; }

void Device_Item::set_parent(Device_Item *item)
{
    if (parent_id() != item->id())
    {
        if (parent_id() && parent_id() != item->id())
            qWarning() << "Device_Item::set_parent diffrent id";
        set_parent_id(item->id());
    }
    parent_ = item;

    if (item && std::find(item->childs_.cbegin(), item->childs_.cend(), this) == item->childs_.cend())
        item->childs_.push_back(this);
}

Device_item_Group *Device_Item::group() const { return group_; }
void Device_Item::set_group(Device_item_Group *group)
{
    if (group_id() != group->id())
    {
        if (group_id())
            qWarning() << "Device_Item::set_group diffrent id";
        set_group_id(group->id());
    }
    if (group_ == group)
        return;
    if (group_)
        group_->remove_item(this);
    group_ = group;
    group_->add_item(this);
}

Device *Device_Item::device() const { return device_; }

void Device_Item::set_device(Device *device)
{
    if (device_id() != device->id())
    {
        if (device_id())
            qWarning() << "Device_Item::set_device diffrent id";
        set_device_id(device->id());
    }
    device_ = device;
    setParent(device);
}

uint8_t Device_Item::register_type() const
{
    return device_ ? device_->type_mng()->register_type(type_id()) : 0;
}

qint64 Device_Item::value_time() const { return data_.timestamp_msecs(); }
uint32_t Device_Item::value_user_id() const { return data_.user_id(); }

const DB::Device_Item_Value &Device_Item::data() const { return data_; }

QVariant Device_Item::value() const
{
    std::lock_guard lock(mutex_);
    return data_.value();
}

void Device_Item::set_display_value(const QVariant &val)
{
    std::lock_guard lock(mutex_);
    const QVariant display_value = data_.value();
    if (is_connected() != val.isValid() || display_value.type() != val.type() || display_value != val)
    {
        data_.set_value(val);
        emit value_changed(0, display_value);
    }
}

bool Device_Item::set_data(const DB::Device_Item_Value &data)
{
    return set_data(data.raw_value(), data.value(), data.user_id(), data.timestamp_msecs());
}

bool Device_Item::set_data(const QVariant &raw, const QVariant &val, uint32_t user_id, qint64 value_time)
{
    std::lock_guard lock(mutex_);
    const QVariant raw_value = data_.raw_value();

    if (raw_value.type() != raw.type() || raw_value != raw ||
        data_.value().type() != val.type() || data_.value() != val)
    {
        data_.set_timestamp_msecs(value_time);
        data_.set_user_id(user_id);
        data_.set_raw_value(raw);
        data_.set_value(val);

        if (data_.item_id() != id())
            data_.set_item_id(id());

        emit value_changed(user_id, raw_value);
        return true;
    }
    return false;
}

QVariant Device_Item::raw_value() const
{
    std::lock_guard lock(mutex_);
    return data_.raw_value();
}

QVariant Device_Item::get_raw_value() const
{
    std::lock_guard lock(mutex_);
    return data_.raw_value();
}

int Device_Item::write(const QVariant& display_value, uint32_t mode_id, uint32_t user_id)
{
    if (!group_)
    {
        qWarning() << "Device_Item::write havnt group";
        return WF_NO_GROUP;
    }

    static const QMetaMethod is_can_change_signal = QMetaMethod::fromSignal(&Device_Item::is_can_change);
    static const QMetaMethod display_to_raw_signal = QMetaMethod::fromSignal(&Device_Item::display_to_raw);

    const QVariant* raw_data = &display_value;

    if (!is_control())
    {
        qCDebug(SchemeDetailLog) << toString() << "Write abort: Not control" << display_value;
        return WF_NOT_CONTROL;
    }
    if (!is_connected())
    {
        qCDebug(SchemeDetailLog) << toString() << "Write abort: Not connected" << display_value;
        return WF_NOT_CONNECTED;
    }
    if (mode_id && group_->mode_id() != mode_id) // Режим автоматизации
    {
        qCDebug(SchemeDetailLog) << toString() << "Write abort: dig mode" << group_->mode_id() << "want" << mode_id << display_value;
        return WF_DIG_MODE;
    }
    if (isSignalConnected(is_can_change_signal) && !is_can_change(display_value, user_id)) // Проверка в скрипте
    {
        qCDebug(SchemeDetailLog) << toString() << "Write abort: Can't change" << display_value;
        return WF_CANT_CHANGE;
    }

    QVariant tmp_raw_data;

    if (isSignalConnected(display_to_raw_signal))
    {
        tmp_raw_data = display_to_raw(display_value);
        raw_data = &tmp_raw_data;
    }

    if (register_type() == Device_Item_Type::RT_FILE && raw_data->type() != QVariant::String)
    {
        QString file_name;
        QByteArray data = raw_data->toByteArray(), file_hash;
        QDataStream ds(data);
        ds >> file_name >> file_hash;

        qCInfo(SchemeDetailLog) << toString() << "Write file to control: " << toString() << file_name << file_hash.toHex();
        set_data(file_hash, file_name);
        return WF_NOT_FAIL;
    }
    else if (data_.raw_value().type() != raw_data->type() || data_.raw_value() != *raw_data
             || register_type() == Device_Item_Type::RT_SIMPLE_BUTTON)
    {
        emit group_->control_state_changed(this, *raw_data, user_id);
        return WF_NOT_FAIL;
    }
    else if (set_raw_value(*raw_data, false, user_id)) // TODO: Check is bug?
        return WF_NOT_FAIL; // do not emit control_state_changed becose raw not changed and item_changed already emited

    qCDebug(SchemeDetailLog) << toString() << "Write abort: Not changed" << display_value;
    return WF_NOT_CHANGED;
}

bool Device_Item::set_raw_value(const QVariant& raw_data, bool force, uint32_t user_id, bool silent, qint64 value_time)
{
    std::lock_guard lock(mutex_);

    static const QMetaMethod raw_to_display_signal = QMetaMethod::fromSignal(&Device_Item::raw_to_display);
    const QVariant display_data = isSignalConnected(raw_to_display_signal) ? raw_to_display(raw_data) : raw_data;

    if (register_type() == Device_Item_Type::RT_SIMPLE_BUTTON)
        force = true;

    const QVariant raw_value = data_.raw_value();

    if (force || raw_data.isValid() != raw_value.isValid() || raw_value.type() != raw_data.type() || raw_value != raw_data
        || data_.value().type() != display_data.type() || data_.value() != display_data)
    {
        data_.set_timestamp_msecs(value_time);
        data_.set_user_id(user_id);
        data_.set_raw_value(raw_data);
        data_.set_value(display_data);

        if (data_.item_id() != id())
            data_.set_item_id(id());

        if (!silent)
        {
            if (connection_state_real() && (!raw_value.isValid() || !data_.raw_value().isValid()))
            {
                emit connection_state_changed(is_connected());
            }

            emit value_changed(user_id, raw_value);
        }
        return true;
    }
    return false;
}

Device_Item *Device_Item::child(int index) const
{
    if (index < childs_.size())
        return childs_.at(index);
    return nullptr;
}

bool Device_Item::connection_state_real() const { return data_.flag(); }
void Device_Item::set_connection_state_real(bool state) { data_.set_flag(state); }

QVector<Device_Item *> Device_Item::childs() const { return childs_; }

bool Device_Item::is_connected() const { return connection_state_real() && data_.raw_value().isValid(); }

bool Device_Item::set_connection_state(bool value, bool silent)
{
    static const QMetaMethod clarify_connection_state_signal = QMetaMethod::fromSignal(&Device_Item::clarify_connection_state);
    if (isSignalConnected(clarify_connection_state_signal))
        value = clarify_connection_state(value);

    if (connection_state_real() != value)
    {
        set_connection_state_real(value);
        if (data_.raw_value().isValid())
        {
            if (!silent)
            {
                emit connection_state_changed(value);
            }
            return true;
        }
    }
    return false;
}

bool Device_Item::is_control() const
{
    return is_control(register_type());
}

/*static*/ bool Device_Item::is_control(uint register_type)
{
    switch (register_type)
    {
    case Device_Item_Type::RT_COILS:
    case Device_Item_Type::RT_HOLDING_REGISTERS:
    case Device_Item_Type::RT_SIMPLE_BUTTON:
    case Device_Item_Type::RT_FILE:
        return true;
    default:
        return false;
    }
}

QString Device_Item::toString()
{
    std::lock_guard lock(mutex_);

    QString string;
    if (group_ && group_->section())
        string = group_->section()->toString() + ' ';
    string += display_name();
    if (is_connected())
        string += " value " + data_.value().toString() + '(' + data_.raw_value().toString() + ')';
    else
        string += " Не подключен";
    return string;
}

// ---------------------------------------------------------------

QDataStream &operator<<(QDataStream &ds, Device_Item *item)
{
    return ds << *item;
}

} // namespace Das
