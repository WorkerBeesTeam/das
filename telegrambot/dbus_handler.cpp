﻿#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>

#include "worker.h"
#include "informer.h"

#include "dbus_handler.h"

namespace Das {

Dbus_Handler::Dbus_Handler(Worker* worker) :
    worker_(worker)
{
    is_manual_connect_ = true;
}

void Dbus_Handler::connection_state_changed(const Scheme_Info& scheme, uint8_t state)
{
    uint8_t flags = state & CS_FLAGS;
    Q_UNUSED(flags)

    state &= ~CS_FLAGS;

    auto dbg = qDebug(DBus_log) << "Connection state - instance:" << scheme.id() << "state:" << int(state) << "flags:" << int(flags);

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
        dbg << "connected just now";
        if (flags)
            dbg << "(do nothing)";
        else
            worker_->informer_->connected(scheme);
        break;
    case CS_CONNECTED:
        dbg << "connected (do nothing)";
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

void Dbus_Handler::status_changed(const Scheme_Info& scheme, const QVector<DIG_Status> &pack)
{
    worker_->informer_->change_status(scheme, pack);
}

void Dbus_Handler::connect_to(QDBusInterface *iface)
{
#define CONNECT_TO_(a,b,x,y,...) \
    connect(iface, SIGNAL(x(Scheme_Info, __VA_ARGS__)), \
            a, SLOT(y(Scheme_Info, __VA_ARGS__)), b)

#define CONNECT_TO_THIS(x,...) CONNECT_TO_(this, Qt::DirectConnection, x, x, __VA_ARGS__)

    CONNECT_TO_THIS(connection_state_changed, uint8_t);
    CONNECT_TO_THIS(event_message_available, QVector<Log_Event_Item>);
    CONNECT_TO_THIS(status_changed, QVector<DIG_Status>);
}

} // namespace Das
