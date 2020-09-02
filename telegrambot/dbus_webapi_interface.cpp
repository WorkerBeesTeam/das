#include <QDBusInterface>


#include "dbus_webapi_interface.h"

namespace Das {
namespace DBus {

WebApi_Interface_Handler::WebApi_Interface_Handler()
{
    is_manual_connect_ = true;
}

void WebApi_Interface_Handler::connect_to(QDBusInterface *iface)
{
#define CONNECT_TO_(a,b,x,y,...) \
    connect(iface, SIGNAL(x(__VA_ARGS__)), \
            a, SIGNAL(y(__VA_ARGS__)), b)

#define CONNECT_TO_THIS(x,...) CONNECT_TO_(this, Qt::DirectConnection, x, x, __VA_ARGS__)

    CONNECT_TO_THIS(tg_user_authorized, qint64);
}

} // namespace DBus
} // namespace Das
