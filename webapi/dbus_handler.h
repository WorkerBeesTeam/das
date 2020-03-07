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
private:
    void connect_to(QDBusInterface* iface) override;

    Worker* worker_;
};

} // namespace WebApi
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_WEBAPI_DBUS_INTERFACE_H
