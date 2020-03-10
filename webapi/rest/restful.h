#ifndef DAS_REST_RESTFUL_H
#define DAS_REST_RESTFUL_H

#include <thread>

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

struct Config
{
    int thread_count_;
    std::string address_, port_;
};

class Restful
{
public:
    Restful(DBus::Interface* dbus_iface, std::shared_ptr<JWT_Helper> jwt_helper, const Config& config);
    ~Restful();

    void stop();
    void join();
private:
    void run(DBus::Interface* dbus_iface, std::shared_ptr<JWT_Helper> jwt_helper, const Config& config);

    served::net::server* server_;
    std::thread thread_;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_RESTFUL_H