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
}

void Dbus_Handler::connect_to(QDBusInterface *iface)
{
#define CONNECT_TO_WEBSOCK(x,y,...) \
    connect(iface, SIGNAL(x(Scheme_Info, __VA_ARGS__)), \
            worker_->websock_th_->ptr(), SLOT(y(Scheme_Info, __VA_ARGS__)), Qt::QueuedConnection)

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
}

} // namespace WebApi
} // namespace Server
} // namespace Das
