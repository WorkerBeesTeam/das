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

Interface::Interface(Handler_Object* handler, const QString& service_name, const QString& object_path, const QString& interface_name) :
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

Interface::~Interface()
{
    delete_iface();
    if (watcher_)
        delete watcher_;
    delete bus_;
}

void Interface::service_registered(const QString& service)
{
    if (service == service_name_)
        connect_to_interface();
}

void Interface::service_unregistered(const QString& service)
{
    if (service == service_name_)
    {
        delete_iface();
        QMetaObject::invokeMethod(handler_, "connection_state_changed", Qt::QueuedConnection,
                                  Q_ARG(Scheme_Info, {}), Q_ARG(uint8_t, CS_SERVER_DOWN));
//        connection_state_changed({}, CS_SERVER_DOWN);
        // TODO: send CS_SERVER_DOWN state for all web sock
    }
}

void Interface::delete_iface()
{
    if (iface_)
    {
        delete iface_;
        iface_ = nullptr;
    }
}

void Interface::connect_to_interface()
{
    delete_iface();

    iface_ = new QDBusInterface(service_name_, object_path_, interface_name_, *bus_);
    if (iface_->isValid())
    {
        handler_->connect_to(iface_);

#define CONNECT_TO_HANDLER(name, ...) \
    connect(iface_, SIGNAL(name(Scheme_Info, __VA_ARGS__)), \
        handler_, SLOT(name(Scheme_Info, __VA_ARGS__)))

        CONNECT_TO_HANDLER(connection_state_changed, uint8_t);
        CONNECT_TO_HANDLER(device_item_values_available, QVector<Log_Value_Item>);
        CONNECT_TO_HANDLER(event_message_available, QVector<Log_Event_Item>);
        CONNECT_TO_HANDLER(time_info, QTimeZone, qint64);
        CONNECT_TO_HANDLER(structure_changed, QByteArray);
        CONNECT_TO_HANDLER(dig_param_values_changed, QVector<DIG_Param_Value>);
        CONNECT_TO_HANDLER(dig_mode_changed, QVector<DIG_Mode>);
        CONNECT_TO_HANDLER(status_changed, QVector<DIG_Status>);
    }
    else
    {
        qCritical(DBus_log) << "Bad interface:" << iface_->lastError();
        delete_iface();
    }
}

bool Interface::is_connected(uint32_t scheme_id)
{
    if (iface_)
    {
        QDBusReply<bool> reply = iface_->call("is_connected", scheme_id);
        if (reply.isValid())
        {
            return reply.value();
        }
        else
        {
            qWarning(DBus_log) << reply.error();
        }
    }
    return false;
}

uint8_t Interface::get_scheme_connection_state(const std::set<uint32_t>& scheme_group_set, uint32_t scheme_id)
{
    if (iface_)
    {
        QDBusReply<uint8_t> reply = iface_->call("get_scheme_connection_state", QVariant::fromValue(scheme_group_set), scheme_id);
        if (reply.isValid())
        {
            return reply.value();
        }
        else
        {
            qWarning(DBus_log) << reply.error();
        }
    }
    return CS_SERVER_DOWN;
}

uint8_t Interface::get_scheme_connection_state2(uint32_t scheme_id)
{
    if (iface_)
    {
        QDBusReply<uint8_t> reply = iface_->call("get_scheme_connection_state2", scheme_id);
        if (reply.isValid())
        {
            return reply.value();
        }
        else
        {
            qWarning(DBus_log) << reply.error();
        }
    }
    return CS_SERVER_DOWN;
}

Scheme_Status Interface::get_scheme_status(uint32_t scheme_id) const
{
    if (iface_)
    {
        QDBusReply<Scheme_Status> reply = iface_->call("get_scheme_status", scheme_id);
        if (reply.isValid())
        {
            return reply.value();
        }
        else
        {
            qWarning(DBus_log) << reply.error();
        }
    }
    return Scheme_Status{CS_SERVER_DOWN, {}};
}

QVector<Device_Item_Value> Interface::get_device_item_values(uint32_t scheme_id) const
{
    if (iface_)
    {
        QDBusReply<QVector<Device_Item_Value>> reply = iface_->call("get_device_item_values", scheme_id);
        if (reply.isValid())
        {
            return reply.value();
        }
        else
        {
            qWarning(DBus_log) << reply.error();
        }
    }
    return {};
}

void Interface::send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& data)
{
    if (iface_)
    {
        QDBusReply<void> reply = iface_->call("send_message_to_scheme", scheme_id, QVariant::fromValue(ws_cmd), user_id, data);
        if (!reply.isValid())
        {
            qWarning(DBus_log) << reply.error();
        }
    }
}

} // namespace DBus
} // namespace Das
