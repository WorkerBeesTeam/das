#ifndef DAS_REST_BOT_CONTROLLER_H
#define DAS_REST_BOT_CONTROLLER_H

#include <thread>

#include "../webapi/rest/rest_config.h"

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
namespace Bot {

class Controller
{
public:
    Controller(DBus::Interface* dbus_iface, std::shared_ptr<JWT_Helper> jwt_helper, const Config& config);
    ~Controller();

    void stop();
    void join();
private:
    void run(DBus::Interface* dbus_iface, std::shared_ptr<JWT_Helper> jwt_helper, const Config& config);

    served::net::server* server_;
    std::thread thread_;
};

} // namespace Bot
} // namespace Rest
} // namespace Das

#endif // DAS_REST_BOT_CONTROLLER_H
