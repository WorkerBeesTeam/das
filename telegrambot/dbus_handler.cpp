#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>

#include "worker.h"
#include "informer.h"

#include "dbus_handler.h"

namespace Das {

Q_LOGGING_CATEGORY(DBus_log, "DBus")

Dbus_Handler::Dbus_Handler(Worker* worker) :
    worker_(worker)
{
}

void Dbus_Handler::connection_state_changed(const Scheme_Info& scheme, uint8_t state)
{
    uint8_t flags = state & CS_FLAGS;
    Q_UNUSED(flags)

    state &= ~CS_FLAGS;

    auto dbg = qDebug(DBus_log) << "instance:" << scheme.id() << "state:" << int(state);

    switch (state)
    {
    case CS_DISCONNECTED:
        dbg << "disconnected";
        worker_->informer_->disconnected(scheme, false);
        break;
    case CS_DISCONNECTED_JUST_NOW:
        dbg << "disconnected just now";
        worker_->informer_->disconnected(scheme, true);
        break;
    case CS_CONNECTED_JUST_NOW:
        dbg << "connected";
        worker_->informer_->connected(scheme);
        break;
    default:
        break;
    }

//    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "send_connection_state", Qt::QueuedConnection, Q_ARG(Scheme_Info, scheme), Q_ARG(uint8_t, state | flags));
}

void Dbus_Handler::event_message_available(const Scheme_Info &scheme, const QVector<Log_Event_Item> &event_pack)
{
    worker_->informer_->send_event_messages(scheme, event_pack);
}

void Dbus_Handler::status_inserted(const Scheme_Info& scheme, quint32 group_id, quint32 info_id, const QStringList& args)
{
    worker_->informer_->add_status(scheme, {0, group_id, info_id, args});
}

void Dbus_Handler::status_removed(const Scheme_Info& scheme, quint32 group_id, quint32 info_id)
{
    worker_->informer_->remove_status(scheme, {0, group_id, info_id});
}

} // namespace Das
