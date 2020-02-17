#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>

#include "worker.h"

#include "dbus_handler.h"

namespace Das {
namespace Server {
namespace WebApi {

Q_LOGGING_CATEGORY(DBus_log, "DBus")

Dbus_Handler::Dbus_Handler(Worker* worker) :
    worker_(worker)
{
}

void Dbus_Handler::connection_state_changed(const Scheme_Info& scheme, uint8_t state)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "send_connection_state", Qt::QueuedConnection, Q_ARG(Scheme_Info, scheme), Q_ARG(uint8_t, state));
}

void Dbus_Handler::device_item_values_available(const Scheme_Info& scheme, const QVector<Log_Value_Item>& pack)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "sendDevice_ItemValues", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(QVector<Log_Value_Item>, pack));
}

void Dbus_Handler::event_message_available(const Scheme_Info& scheme, const QVector<Log_Event_Item>& event_pack)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "sendEventMessage", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(QVector<Log_Event_Item>, event_pack));
}

void Dbus_Handler::time_info(const Scheme_Info& scheme, const QTimeZone& tz, qint64 time_offset)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "send_time_info", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(QTimeZone, tz), Q_ARG(qint64, time_offset));
}

void Dbus_Handler::structure_changed(const Scheme_Info& scheme, const QByteArray& data)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "send_structure_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(QByteArray, data));
}

void Dbus_Handler::dig_param_values_changed(const Scheme_Info& scheme, const QVector<DIG_Param_Value>& pack)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "send_dig_param_values_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(QVector<DIG_Param_Value>, pack));
}

void Dbus_Handler::dig_mode_changed(const Scheme_Info& scheme, const DIG_Mode &mode)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "sendModeChanged", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(DIG_Mode, mode));
}

void Dbus_Handler::status_inserted(const Scheme_Info& scheme, uint32_t group_id, uint32_t info_id, const QStringList& args)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "sendStatusInserted", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(uint32_t, group_id), Q_ARG(uint32_t, info_id), Q_ARG(QStringList, args));
}

void Dbus_Handler::status_removed(const Scheme_Info& scheme, uint32_t group_id, uint32_t info_id)
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "sendStatusRemoved", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, scheme), Q_ARG(uint32_t, group_id), Q_ARG(uint32_t, info_id));
}

} // namespace WebApi
} // namespace Server
} // namespace Das
