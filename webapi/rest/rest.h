#ifndef DAS_REST_RESTFUL_H
#define DAS_REST_RESTFUL_H

#include <thread>

#include <QLoggingCategory>

#include "rest_config.h"

namespace served {
namespace net {
class server;
} // namespace net
} // namespace served

namespace Das {
namespace DBus {
class Interface;
} // namespace Dbus

class JWT_Helper;

namespace Rest {

Q_DECLARE_LOGGING_CATEGORY(Rest_Log)

class Restful
{
public:
    Restful(DBus::Interface* dbus_iface, const Config& config);
    ~Restful();

    void stop();
    void join();
private:
    void run(DBus::Interface* dbus_iface, const Config& config);

    served::net::server* server_;
    std::thread thread_;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_RESTFUL_H
