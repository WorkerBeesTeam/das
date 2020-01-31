#ifndef DAS_DBUS_COMMON_H
#define DAS_DBUS_COMMON_H

#include <set>

#include <Das/db/dig_status.h>

#define DAS_DBUS_DEFAULT_SERVICE "ru.deviceaccess.Das"
#define DAS_DBUS_DEFAULT_SERVICE_SERVER DAS_DBUS_DEFAULT_SERVICE".Server"
#define DAS_DBUS_DEFAULT_SERVICE_CLIENT DAS_DBUS_DEFAULT_SERVICE".Client"
#define DAS_DBUS_DEFAULT_OBJECT "/"
#define DAS_DBUS_DEFAULT_INTERFACE DAS_DBUS_DEFAULT_SERVICE".iface"

namespace Das {

struct Scheme_Status
{
    uint8_t connection_state_;
    std::set<DIG_Status> status_set_;
};

namespace DBus {

void register_dbus_types();

} // namespace DBus
} // namespace Das

Q_DECLARE_METATYPE(Das::Scheme_Status)

#endif // DAS_SERVER_DBUS_COMMON_H
