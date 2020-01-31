#include <QtDBus>

#include <Helpz/dtls_server.h>

#include <Das/commands.h>

#include "dbus_object_base.h"

namespace Das {
namespace DBus {

Object_Base::Object_Base(const QString& service_name, const QString& object_path) :
    QObject(),
    service_name_(service_name), object_path_(object_path)
{
    DBus::register_dbus_types();

    if (!QDBusConnection::systemBus().isConnected())
    {
        qCritical() << "DBus не доступен";
    }
    else if (!QDBusConnection::systemBus().registerService(service_name))
    {
        qCritical() << "DBus служба уже зарегистрированна" << service_name;
    }
    else if (!QDBusConnection::systemBus().registerObject(object_path, this, QDBusConnection::ExportAllContents))
    {
        qCritical() << "DBus объект уже зарегистрирован" << object_path;
    }
    else
    {
        qInfo() << "Dbus служба успешно зарегистрированна" << service_name << object_path;
    }
}

Object_Base::~Object_Base()
{
    if (QDBusConnection::systemBus().isConnected())
    {
        QDBusConnection::systemBus().unregisterObject(object_path_);
        QDBusConnection::systemBus().unregisterService(service_name_);
    }
}

uint16_t Object_Base::cmd_from_web_command(quint8 cmd, int proto_version) const
{
    if (proto_version != 204)
    {
        qWarning() << "cmd_from_web_command proto version:" << proto_version;
//        return 0;
    }

    switch (cmd)
    {
    case Das::WS_WRITE_TO_DEV_ITEM:         return Ver_2_4::Cmd::WRITE_TO_ITEM;
    case Das::WS_CHANGE_DIG_MODE_ITEM:         return Ver_2_4::Cmd::SET_MODE;
    case Das::WS_CHANGE_DIG_PARAM_VALUES: return Ver_2_4::Cmd::SET_DIG_PARAM_VALUES;
    case Das::WS_EXEC_SCRIPT:               return Ver_2_4::Cmd::EXEC_SCRIPT_COMMAND;
    case Das::WS_RESTART:                   return Ver_2_4::Cmd::RESTART;
    case Das::WS_STRUCT_MODIFY:             return Ver_2_4::Cmd::MODIFY_SCHEME;
    default: break;
    }
    return 0;
}

} // namespace DBus
} // namespace Das
