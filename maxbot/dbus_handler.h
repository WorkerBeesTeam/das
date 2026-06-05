#ifndef DAS_SERVER_WEBAPI_DBUS_INTERFACE_H
#define DAS_SERVER_WEBAPI_DBUS_INTERFACE_H

#include <QObject>

#include <Das/db/dig_status_type.h>
#include <Das/db/dig_param_value.h>
#include <Das/log/log_pack.h>

#include <plus/das/scheme_info.h>
#include <dbus/dbus_interface.h>

namespace Das {

class Worker;

class Dbus_Handler : public DBus::Handler_Object
{
    Q_OBJECT
public:
    Dbus_Handler(Worker* worker);
private slots:
    void connection_state_changed(const Scheme_Info& scheme, uint8_t state);
    void event_message_available(const Scheme_Info& scheme, const QVector<Log_Event_Item>& event_pack);
    void status_changed(const Scheme_Info& scheme, const QVector<DIG_Status> &pack);
private:
    void connect_to(QDBusInterface* iface) override;
    Worker* worker_;
};

} // namespace Das

#endif // DAS_SERVER_WEBAPI_DBUS_INTERFACE_H
