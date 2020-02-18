#ifndef DAS_SERVER_WEBAPI_DBUS_INTERFACE_H
#define DAS_SERVER_WEBAPI_DBUS_INTERFACE_H

#include <QObject>

#include <Das/db/dig_status.h>
#include <Das/db/dig_param_value.h>
#include <Das/log/log_pack.h>

#include <plus/das/scheme_info.h>
#include <dbus/dbus_interface.h>

namespace Das {
namespace Server {
namespace WebApi {

class Worker;

class Dbus_Handler : public DBus::Handler_Object
{
    Q_OBJECT
public:
    Dbus_Handler(Worker* worker);
public slots:
    void connection_state_changed(const Scheme_Info& scheme, uint8_t state);

    void device_item_values_available(const Scheme_Info& scheme, const QVector<Log_Value_Item>& pack);
    void event_message_available(const Scheme_Info& scheme, const QVector<Log_Event_Item>& event_pack);
    void time_info(const Scheme_Info& scheme, const QTimeZone& tz, qint64 time_offset);

    void structure_changed(const Scheme_Info& scheme, const QByteArray& data);
    void dig_param_values_changed(const Scheme_Info& scheme, const QVector<DIG_Param_Value> &pack);
    void dig_mode_changed(const Scheme_Info& scheme, const QVector<DIG_Mode> &pack);
    void status_changed(const Scheme_Info& scheme, const QVector<DIG_Status> &pack);
private:
    Worker* worker_;
};

} // namespace WebApi
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_WEBAPI_DBUS_INTERFACE_H
