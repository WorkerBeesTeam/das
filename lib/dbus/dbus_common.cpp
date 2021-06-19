#include <type_traits>

#include <QDBusMetaType>
#include <QTimeZone>
#include <QVector>
#include <QDebug>

//#include <Das/db/dig_status.h>
#include <Das/db/dig_param_value.h>
#include <Das/db/dig_status.h>
#include <Das/db/dig_mode.h>
#include <Das/db/device_item_value.h>
#include <Das/log/log_pack.h>
#include <plus/das/scheme_info.h>

#include "dbus_common.h"

using DIG_Param_Value = Das::DIG_Param_Value;
using DIG_Status = Das::DIG_Status;
using DIG_Mode = Das::DIG_Mode;
using Scheme_Status = Das::Scheme_Status;
using Device_Item_Value = Das::Device_Item_Value;

Q_DECLARE_METATYPE(QTimeZone)
Q_DECLARE_METATYPE(DIG_Param_Value)
Q_DECLARE_METATYPE(DIG_Status)
Q_DECLARE_METATYPE(DIG_Mode)
Q_DECLARE_METATYPE(QVector<Log_Value_Item>)
Q_DECLARE_METATYPE(QVector<Log_Event_Item>)
Q_DECLARE_METATYPE(QVector<DIG_Param_Value>)
Q_DECLARE_METATYPE(QVector<DIG_Status>)
Q_DECLARE_METATYPE(QVector<DIG_Mode>)
Q_DECLARE_METATYPE(QVector<Device_Item_Value>)

QDBusArgument &operator<<(QDBusArgument &arg, const QTimeZone &item)
{
    arg.beginStructure();
    arg << item.id();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QTimeZone &item)
{
    arg.beginStructure();
    QByteArray data;
    arg >> data;
    arg.endStructure();
    item = QTimeZone(data);
    return arg;
}

template<typename T>
inline const QDBusArgument &operator>>(const QDBusArgument &arg, std::set<T> &list)
{
    arg.beginArray();
    list.clear();
    while (!arg.atEnd()) {
        T item;
        arg >> item;
        list.insert(item);
    }

    arg.endArray();
    return arg;
}

template<typename T, typename O, typename OB>
void load_to_setter(const QDBusArgument &arg, O& obj, void (OB::*setter)(T))
{
    typename std::decay<T>::type value;
    arg >> value;
    (obj.*setter)(std::move(value));
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::Scheme_Info &item)
{
    arg.beginStructure();
    arg << item.id() << item.extending_scheme_ids() << item.scheme_groups();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::Scheme_Info &item)
{
    arg.beginStructure();
    load_to_setter(arg, item, &Das::Scheme_Info::set_id);
    load_to_setter(arg, item, static_cast<void(Das::Scheme_Info::*)(std::set<uint32_t>&&)>(&Das::Scheme_Info::set_extending_scheme_ids));
    load_to_setter(arg, item, static_cast<void(Das::Scheme_Info::*)(std::set<uint32_t>&&)>(&Das::Scheme_Info::set_scheme_groups));
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::DB::Log_Base_Item &item)
{
    qint64 timestamp = item.timestamp_msecs();
    if (item.flag())
        timestamp |= Das::DB::Log_Base_Item::LOG_FLAG;
    arg << timestamp << item.user_id();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::DB::Log_Base_Item &item)
{
    qint64 timestamp;
    arg >> timestamp;

    item.set_flag(timestamp & Das::DB::Log_Base_Item::LOG_FLAG);
    item.set_timestamp_msecs(timestamp & ~Das::DB::Log_Base_Item::LOG_FLAG);

    load_to_setter(arg, item, &Das::DB::Log_Base_Item::set_user_id);
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::DB::Log_Value_Item &item)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << item.raw_value() << item.value();

    arg.beginStructure();
    arg << static_cast<const Das::DB::Log_Base_Item&>(item) << data << item.item_id();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::DB::Log_Value_Item &item)
{
    QByteArray data;
    QVariant raw_value, value;

    arg.beginStructure();
    arg >> static_cast<Das::DB::Log_Base_Item&>(item) >> data;
    QDataStream ds(&data, QIODevice::ReadOnly);
    ds >> raw_value >> value;

    load_to_setter(arg, item, &Das::DB::Log_Value_Item::set_item_id);
    arg.endStructure();

    item.set_raw_value(raw_value);
    item.set_value(value);
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::DB::Log_Event_Item &item)
{
    arg.beginStructure();
    arg << static_cast<const Das::DB::Log_Base_Item&>(item) << item.type_id() << item.category() << item.text();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::DB::Log_Event_Item &item)
{
    arg.beginStructure();
    arg >> static_cast<Das::DB::Log_Base_Item&>(item);
    load_to_setter(arg, item, &Das::DB::Log_Event_Item::set_type_id);
    load_to_setter(arg, item, &Das::DB::Log_Event_Item::set_category);
    load_to_setter(arg, item, &Das::DB::Log_Event_Item::set_text);
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::DIG_Param_Value &item)
{
    arg.beginStructure();
    arg << static_cast<const Das::DB::Log_Base_Item&>(item) << item.group_param_id() << item.value();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::DIG_Param_Value &item)
{
    arg.beginStructure();
    arg >> static_cast<Das::DB::Log_Base_Item&>(item);
    load_to_setter(arg, item, &Das::DIG_Param_Value::set_group_param_id);
    load_to_setter(arg, item, &Das::DIG_Param_Value::set_value);
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::DIG_Status &item)
{
    arg.beginStructure();
    arg << static_cast<const Das::DB::Log_Base_Item&>(item) << item.group_id() << item.status_id() << item.args();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::DIG_Status &item)
{
    arg.beginStructure();
    arg >> static_cast<Das::DB::Log_Base_Item&>(item);
    load_to_setter(arg, item, &Das::DIG_Status::set_group_id);
    load_to_setter(arg, item, &Das::DIG_Status::set_status_id);
    load_to_setter(arg, item, &Das::DIG_Status::set_args);
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::DIG_Mode &item)
{
    arg.beginStructure();
    arg << static_cast<const Das::DB::Log_Base_Item&>(item) << item.group_id() << item.mode_id();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::DIG_Mode &item)
{
    arg.beginStructure();
    arg >> static_cast<Das::DB::Log_Base_Item&>(item);
    load_to_setter(arg, item, &Das::DIG_Mode::set_group_id);
    load_to_setter(arg, item, &Das::DIG_Mode::set_mode_id);
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::Device_Item_Value &item)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << item.raw_value() << item.value();

    arg.beginStructure();
    arg << static_cast<const Das::DB::Log_Base_Item&>(item) << item.item_id() << data;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::Device_Item_Value &item)
{
    QByteArray data;

    arg.beginStructure();
    arg >> static_cast<Das::DB::Log_Base_Item&>(item);
    load_to_setter(arg, item, &Das::Device_Item_Value::set_item_id);
    arg >> data;
    arg.endStructure();

    QVariant raw_value, value;
    QDataStream ds(&data, QIODevice::ReadOnly);
    ds >> raw_value >> value;

    item.set_raw_value(raw_value);
    item.set_value(value);
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::Scheme_Status &item)
{
    arg.beginStructure();
    arg << item.connection_state_ << item.status_set_;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::Scheme_Status &item)
{
    arg.beginStructure();
    arg >> item.connection_state_ >> item.status_set_;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Das::Scheme_Time_Info &item)
{
    arg.beginStructure();
    arg << static_cast<qint64>(item._utc_time) << item._tz_offset << QString::fromStdString(item._tz_name);
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Das::Scheme_Time_Info &item)
{
    QString tz_name;
    arg.beginStructure();
    arg >> reinterpret_cast<qint64&>(item._utc_time) >> item._tz_offset >> tz_name;
    arg.endStructure();
    item._tz_name = tz_name.toStdString();
    return arg;
}

namespace Das {

Q_LOGGING_CATEGORY(DBus_log, "DBus")

namespace DBus {

#define REGISTER_PARAM(x) \
    qRegisterMetaType<x>(#x); \
    qDBusRegisterMetaType<x>()

void register_dbus_types()
{
    REGISTER_PARAM(QTimeZone);
    REGISTER_PARAM(uint8_t);
    REGISTER_PARAM(uint16_t);
    REGISTER_PARAM(uint32_t);
    REGISTER_PARAM(int32_t);
    REGISTER_PARAM(std::vector<uint32_t>);
    REGISTER_PARAM(std::set<uint32_t>);
    REGISTER_PARAM(Scheme_Info);
    REGISTER_PARAM(Log_Value_Item);
    REGISTER_PARAM(Log_Event_Item);
    REGISTER_PARAM(DIG_Param_Value);
    REGISTER_PARAM(DIG_Status);
    REGISTER_PARAM(DIG_Mode);
    REGISTER_PARAM(Device_Item_Value);
    REGISTER_PARAM(QVector<Log_Value_Item>);
    REGISTER_PARAM(QVector<Log_Event_Item>);
    REGISTER_PARAM(QVector<DIG_Param_Value>);
    REGISTER_PARAM(QVector<DIG_Status>);
    REGISTER_PARAM(QVector<DIG_Mode>);
    REGISTER_PARAM(QVector<Device_Item_Value>);
    REGISTER_PARAM(Scheme_Status);
    REGISTER_PARAM(Scheme_Time_Info);
}

} // namespace DBus
} // namespace Das
