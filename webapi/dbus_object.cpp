#include <QtDBus>

#include "dbus_object.h"

namespace Das {
namespace Server {
namespace WebApi {

/*static*/ Dbus_Object* Dbus_Object::_self = nullptr;
/*static*/ Dbus_Object *Dbus_Object::instance() { return _self; }

Dbus_Object::Dbus_Object(const QString &service_name, const QString &object_path) :
    QObject(),
    service_name_(service_name), object_path_(object_path)
{
    if (_self)
        qCCritical(DBus_log) << "Dbus_Object already allocated";
    _self = this;

    if (!QDBusConnection::systemBus().isConnected())
    {
        qCCritical(DBus_log) << "DBus не доступен";
    }
    else if (!QDBusConnection::systemBus().registerService(service_name))
    {
        qCCritical(DBus_log) << "DBus служба уже зарегистрированна" << service_name;
    }
    else if (!QDBusConnection::systemBus().registerObject(object_path, this, QDBusConnection::ExportAllContents))
    {
        qCCritical(DBus_log) << "DBus объект уже зарегистрирован" << object_path;
    }
    else
    {
        qCInfo(DBus_log) << "Dbus служба успешно зарегистрированна" << service_name << object_path;
    }
}

Dbus_Object::~Dbus_Object()
{
    if (QDBusConnection::systemBus().isConnected())
    {
        QDBusConnection::systemBus().unregisterObject(object_path_);
        QDBusConnection::systemBus().unregisterService(service_name_);
    }
}

} // namespace WebApi
} // namespace Server
} // namespace Das
