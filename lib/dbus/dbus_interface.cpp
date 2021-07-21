#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>

#include "dbus_interface.h"

namespace Das {
namespace DBus {

Q_LOGGING_CATEGORY(DBus_log, "DBus")

Interface_Impl::Interface_Impl(Handler_Object* handler, const QString& service_name, const QString& object_path, const QString& interface_name) :
    iface_(nullptr),
    watcher_(nullptr),
    handler_(handler), service_name_(service_name), object_path_(object_path), interface_name_(interface_name)
{
    register_dbus_types();

    bus_ = new QDBusConnection(QDBusConnection::systemBus());
    if (bus_->isConnected())
    {
        watcher_ = new QDBusServiceWatcher(service_name_, *bus_);
        connect(watcher_, &QDBusServiceWatcher::serviceRegistered,   this, &Interface::service_registered);
        connect(watcher_, &QDBusServiceWatcher::serviceUnregistered, this, &Interface::service_unregistered);

        QStringList service_names = bus_->interface()->registeredServiceNames();
        for (const QString& name: service_names)
        {
            if (name == service_name)
            {
                connect_to_interface();
                return;
            }
        }
    }
    else
    {
        qCritical(DBus_log) << "Fail connect:" << bus_->lastError();
    }
}

Interface_Impl::~Interface_Impl()
{
    delete_iface();
    if (watcher_)
        delete watcher_;
    delete bus_;
}

void Interface_Impl::service_registered(const QString& service)
{
    if (service == service_name_)
        connect_to_interface();
}

void Interface_Impl::service_unregistered(const QString& service)
{
    if (service == service_name_)
    {
        delete_iface();
        if (handler_) handler_->server_down();
    }
}

void Interface_Impl::delete_iface()
{
    if (iface_)
    {
        delete iface_;
        iface_ = nullptr;
    }
}

void Interface_Impl::connect_to_interface()
{
    delete_iface();

    iface_ = new QDBusInterface(service_name_, object_path_, interface_name_, *bus_);
    if (iface_->isValid())
    {
        if (handler_)
        {
            handler_->connect_to(iface_);

            if (handler_->is_manual_connect_)
                return;

            connect_handler(handler_);
        }
    }
    else
    {
        qCritical(DBus_log) << "Bad interface:" << iface_->lastError();
        delete_iface();
    }
}

template<typename Ret_Type, typename... Args>
Ret_Type Interface_Impl::call_iface(const QString& name, const typename Non_Void<Ret_Type>::type ret, Args... args) const
{
    if (iface_)
    {
        QDBusReply<Ret_Type> reply = iface_->call(name, args...);
        if (reply.isValid())
        {
            if constexpr (!std::is_same<void, Ret_Type>::value)
                return reply.value();
        }
        else
            qWarning(DBus_log) << "Call iface function" << name << "failed:" << reply.error();
    }

    return static_cast<Ret_Type>(ret);
}

template<typename... Args>
void Interface_Impl::call_iface_void(const QString& name, Args... args)
{
    if (iface_)
    {
        QDBusReply<void> reply = iface_->call(name, args...);
        if (!reply.isValid())
            qWarning(DBus_log) << "Call iface function" << name << "failed:" << reply.error();
    }
}

// Interface

void Interface::connect_handler(Handler_Object *handler)
{
#define CONNECT_TO_HANDLER(name, ...) \
    connect(iface_, SIGNAL(name(Scheme_Info, __VA_ARGS__)), \
        handler, SLOT(name(Scheme_Info, __VA_ARGS__)))

        CONNECT_TO_HANDLER(connection_state_changed, uint8_t);
        CONNECT_TO_HANDLER(device_item_values_available, QVector<Log_Value_Item>);
        CONNECT_TO_HANDLER(event_message_available, QVector<Log_Event_Item>);
        CONNECT_TO_HANDLER(time_info, QTimeZone, qint64);
        CONNECT_TO_HANDLER(structure_changed, QByteArray);
        CONNECT_TO_HANDLER(dig_param_values_changed, QVector<DIG_Param_Value>);
        CONNECT_TO_HANDLER(dig_mode_changed, QVector<DIG_Mode>);
        CONNECT_TO_HANDLER(status_changed, QVector<DIG_Status>);
}

bool Interface::is_connected(uint32_t scheme_id)
{
    return call_iface<bool>("is_connected", false, scheme_id);
}

uint8_t Interface::get_scheme_connection_state(const std::set<uint32_t>& scheme_group_set, uint32_t scheme_id)
{
    return call_iface<uint8_t>("get_scheme_connection_state", CS_SERVER_DOWN, QVariant::fromValue(scheme_group_set), scheme_id);
}

uint8_t Interface::get_scheme_connection_state2(uint32_t scheme_id)
{
    return call_iface<uint8_t>("get_scheme_connection_state2", CS_SERVER_DOWN, scheme_id);
}

Scheme_Status Interface::get_scheme_status(uint32_t scheme_id) const
{
    return call_iface<Scheme_Status>("get_scheme_status", Scheme_Status{CS_SERVER_DOWN, {}}, scheme_id);
}

Scheme_Time_Info Interface::get_time_info(uint32_t scheme_id) const
{
    return call_iface<Scheme_Time_Info>("get_time_info", Scheme_Time_Info{0, 0}, scheme_id);
}

void Interface::set_scheme_name(uint32_t scheme_id, uint32_t user_id, const QString &name)
{
    call_iface<void>("set_scheme_name", nullptr, scheme_id, user_id, name);
}

QVector<Device_Item_Value> Interface::get_device_item_values(uint32_t scheme_id) const
{
    return call_iface<QVector<Device_Item_Value>>("get_device_item_values", {}, scheme_id);
}

void Interface::send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& data)
{
    call_iface<void>("send_message_to_scheme", nullptr, scheme_id, QVariant::fromValue(ws_cmd), user_id, data);
}

void Interface::start_stream(uint32_t scheme_id, uint32_t user_id, uint32_t dev_item_id, const QString &url)
{
    call_iface<void>("start_stream", nullptr, scheme_id, user_id, dev_item_id, url);
}

} // namespace DBus
} // namespace Das
