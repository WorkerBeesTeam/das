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

Dbus_Handler::Dbus_Handler(Worker* worker) :
    worker_(worker)
{
    is_manual_connect_ = true;
}

void Dbus_Handler::set_stream_param(const Scheme_Info& scheme, uint32_t dev_item_id, const QByteArray& data)
{
    worker_->stream_server_->set_param(scheme.id(), dev_item_id, data);
}

void Dbus_Handler::connect_to(QDBusInterface *iface)
{
#define CONNECT_TO_(a,b,x,y,...) \
    connect(iface, SIGNAL(x(Scheme_Info, __VA_ARGS__)), \
            a, SLOT(y(Scheme_Info, __VA_ARGS__)), b)

#define CONNECT_TO_WEBSOCK(x,y,...) CONNECT_TO_(worker_->websock_th_->ptr(), Qt::QueuedConnection, x, y, __VA_ARGS__)
#define CONNECT_TO_THIS(x,y,...) CONNECT_TO_(this, Qt::DirectConnection, x, y, __VA_ARGS__)

    CONNECT_TO_WEBSOCK(connection_state_changed, send_connection_state, uint8_t);
    CONNECT_TO_WEBSOCK(device_item_values_available, sendDevice_ItemValues, QVector<Log_Value_Item>);
    CONNECT_TO_WEBSOCK(event_message_available, sendEventMessage, QVector<Log_Event_Item>);
    CONNECT_TO_WEBSOCK(time_info, send_time_info, QTimeZone, qint64);
    CONNECT_TO_WEBSOCK(structure_changed, send_structure_changed, QByteArray);
    CONNECT_TO_WEBSOCK(dig_param_values_changed, send_dig_param_values_changed, QVector<DIG_Param_Value>);
    CONNECT_TO_WEBSOCK(dig_mode_changed, send_dig_mode_pack, QVector<DIG_Mode>);
    CONNECT_TO_WEBSOCK(status_changed, send_dig_status_changed, QVector<DIG_Status>);
    CONNECT_TO_WEBSOCK(stream_toggled, send_stream_toggled, uint32_t, uint32_t, bool);
    CONNECT_TO_WEBSOCK(stream_data, send_stream_data, uint32_t, QByteArray);
    CONNECT_TO_THIS(stream_param, set_stream_param, uint32_t, QByteArray);
}

void Dbus_Handler::server_down()
{
    QMetaObject::invokeMethod(worker_->websock_th_->ptr(), "send_connection_state", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, {}), Q_ARG(uint8_t, CS_SERVER_DOWN));
    // TODO: send CS_SERVER_DOWN state for all web sock
}

} // namespace WebApi
} // namespace Server
} // namespace Das
