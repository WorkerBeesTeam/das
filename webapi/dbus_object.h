#ifndef DAS_WEBAPI_DBUS_OBJECT_H
#define DAS_WEBAPI_DBUS_OBJECT_H

#include <QDBusContext>

#include <dbus/dbus_common.h>

namespace Das {
namespace Server {
namespace WebApi {

class Dbus_Object : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DAS_DBUS_DEFAULT_WEBAPI_INTERFACE)
public:
    static Dbus_Object* instance();

    Dbus_Object(const QString& service_name = DAS_DBUS_DEFAULT_SERVICE".WebApi",
                const QString& object_path = DAS_DBUS_DEFAULT_OBJECT);
    ~Dbus_Object();

signals:
    void tg_user_authorized(qint64 tg_user_id);
public slots:
private:
    static Dbus_Object* _self;
    QString service_name_, object_path_;
};

} // namespace WebApi
} // namespace Server
} // namespace Das

#endif // DAS_WEBAPI_DBUS_OBJECT_H
