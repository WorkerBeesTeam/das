#ifndef DAS_DBUS_COMMON_H
#define DAS_DBUS_COMMON_H

#include <QLoggingCategory>

#include <set>

#include <Das/db/dig_status.h>

#define DAS_DBUS_DEFAULT_SERVICE "ru.deviceaccess.Das"
#define DAS_DBUS_DEFAULT_SERVICE_SERVER DAS_DBUS_DEFAULT_SERVICE".Server"
#define DAS_DBUS_DEFAULT_SERVICE_CLIENT DAS_DBUS_DEFAULT_SERVICE".Client"
#define DAS_DBUS_DEFAULT_OBJECT "/"
#define DAS_DBUS_DEFAULT_INTERFACE DAS_DBUS_DEFAULT_SERVICE".iface"
#define DAS_DBUS_DEFAULT_WEBAPI_INTERFACE DAS_DBUS_DEFAULT_SERVICE".webapiiface"

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(DBus_log)

struct Scheme_Status
{
    uint8_t connection_state_;
    std::set<DIG_Status> status_set_;
};

struct Scheme_Time_Info
{
    int32_t _tz_offset; // Seconds
    int64_t _utc_time; // Milliseconds
    std::string _tz_name;
};

namespace DBus {

void register_dbus_types();

} // namespace DBus
} // namespace Das

Q_DECLARE_METATYPE(Das::Scheme_Status)
Q_DECLARE_METATYPE(Das::Scheme_Time_Info)

#endif // DAS_SERVER_DBUS_COMMON_H
