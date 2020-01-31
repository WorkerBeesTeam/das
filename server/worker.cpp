#include <iostream>
//#include <filesystem>

#include <boost/asio/io_context.hpp>

#include <botan-2/botan/tls_server.h>
#include <botan-2/botan/tls_callbacks.h>
#include <QDir>

#include <Helpz/dtls_server.h>
#include <Helpz/dtls_server_thread.h>
#include <Helpz/settingshelper.h>
#include <Helpz/dtls_tools.h>

//--------
#include "dbus_object.h"
#include "worker.h"

namespace Das {
namespace Server {
using boost::asio::ip::udp;
namespace Z = Helpz;

Worker::Worker(QObject *parent) :
    QObject(parent),
    cl_parser_(this)
{
    qRegisterMetaType<Log_Value_Item>("Log_Value_Item");
    qRegisterMetaType<Log_Event_Item>("Log_Event_Item");
    qRegisterMetaType<QVector<Log_Value_Item>>("QVector<Log_Value_Item>");
    qRegisterMetaType<QVector<Log_Event_Item>>("QVector<Log_Event_Item>");

    QSettings s(qApp->applicationDirPath() + QDir::separator() + qApp->applicationName() + ".conf", QSettings::NativeFormat);

    init_logging(&s);
    init_database(&s);
    init_server(&s);
    init_dbus(&s);

    connect(this, &Worker::processCommands, &cl_parser_, &Command_Line_Parser::process_commands);

    connect(&timer_, &QTimer::timeout, this, &Worker::on_timer);
    timer_.setInterval(60000);
    timer_.setSingleShot(false);
    timer_.start();
}

Worker::~Worker()
{
}

void Worker::init_logging(QSettings *s)
{
    std::tuple<bool, bool> t = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Z::Param<bool>,Z::Param<bool>>
        #endif
            (
            s, "Log",
            Z::Param<bool>{"Debug", false},
            Z::Param<bool>{"Syslog", true}
    )();

    logg().set_debug(std::get<0>(t));
#ifdef Q_OS_UNIX
    logg().set_syslog(std::get<1>(t));
#endif
    QMetaObject::invokeMethod(&logg(), "init", Qt::QueuedConnection);
}

void Worker::on_timer()
{
    auto now = std::chrono::system_clock::now();
    std::lock_guard lock(recently_connected_.mutex_);
    recently_connected_.scheme_id_vect_.erase(std::remove_if(recently_connected_.scheme_id_vect_.begin(), recently_connected_.scheme_id_vect_.end(),
                                                           [this, &now](const Recently_Connected::Recent_Client& item)
    {
        if ((now - item.disconnect_time_) >= std::chrono::minutes(15))
        {
            dbus_->connection_state_changed(item.scheme_, CS_DISCONNECTED);
            return true;
        }
        return false;
    }), recently_connected_.scheme_id_vect_.end());
}

} // namespace Server
} // namespace Das
