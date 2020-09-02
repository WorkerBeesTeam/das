#ifndef DAS_DBUS_WEBAPI_INTERFACE_H
#define DAS_DBUS_WEBAPI_INTERFACE_H

#include <dbus/dbus_interface.h>

namespace Das {
namespace DBus {

class WebApi_Interface_Handler : public Handler_Object
{
    Q_OBJECT
public:
    WebApi_Interface_Handler();
signals:
    void tg_user_authorized(qint64 tg_user_id);
private:
    void connect_to(QDBusInterface* iface) override;
};

} // namespace DBus
} // namespace Das

#endif // DAS_DBUS_WEBAPI_INTERFACE_H
