#include <iostream>
//#include <filesystem>

#include <boost/asio/io_context.hpp>

#include <botan-2/botan/tls_server.h>
#include <botan-2/botan/tls_callbacks.h>

#include <QDir>
#include <QCommandLineParser>

#include <Helpz/dtls_server.h>
#include <Helpz/dtls_server_thread.h>
#include <Helpz/settingshelper.h>
#include <Helpz/dtls_tools.h>

//--------
#include "bot/controller.h"
#include "dbus_handler.h"
#include "informer.h"
#include "worker.h"

namespace Das {

using boost::asio::ip::udp;
namespace Z = Helpz;

Worker::Worker(QObject *parent) :
    QObject(parent)
{
    QSettings s(qApp->applicationDirPath() + QDir::separator() + qApp->applicationName() + ".conf", QSettings::NativeFormat);

    init_logging(&s);
    init_database(&s);
    init_dbus_interface(&s);
    init_bot(&s);
    init_informer(&s);
}

Worker::~Worker()
{
    delete dbus_;
    delete dbus_handler_;
    delete informer_;
    bot_->stop();
    bot_->quit();
    bot_->wait();
    delete bot_;
    delete db_pending_thread_;
    delete db_conn_info_;
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

void Worker::init_database(QSettings* s)
{
    db_conn_info_ = Helpz::SettingsHelper(
                s, "Database",
                Helpz::Param{"Name", "das"},
                Helpz::Param{"User", "das"},
                Helpz::Param{"Password", QString()},
                Helpz::Param{"Host", "localhost"},
                Helpz::Param{"Port", 3306},
                Helpz::Param{"Prefix", "das_"},
                Helpz::Param{"Driver", "QMYSQL"}, // QPSQL
                Helpz::Param{"ConnectOptions", "CLIENT_FOUND_ROWS=1;MYSQL_OPT_RECONNECT=1"}
                ).ptr<Helpz::DB::Connection_Info>();

    Helpz::DB::Connection_Info::set_common(*db_conn_info_);
    db_pending_thread_ = new Helpz::DB::Thread{Helpz::DB::Connection_Info(*db_conn_info_)};
}

void Worker::init_bot(QSettings* s)
{
//    SMTP_Config smtp_config = Helpz::SettingsHelper(
//                s, "SMTP_Client",
//                Helpz::Param{"Server", "mail.example.org"},
//                Helpz::Param{"Port", uint16_t(25)},
//                Helpz::Param{"ConnectionType", "TCP"},
//                Helpz::Param{"UserEmail", QString()},
//                Helpz::Param{"User", QString()},
//                Helpz::Param{"Password", QString()},
//                Helpz::Param{"AuthMethod", "LOGIN"},
//                Helpz::Param{"ConnectionTimeout", uint32_t(5000)},
//                Helpz::Param{"ResponseTimeout", uint32_t(5000)}
//                ).obj<SMTP_Config>();

    bot_ = Helpz::SettingsHelper(
        s, "Bot",
        dbus_,
        Helpz::Param<std::string>{"Token", std::string()},
        Helpz::Param<std::string>{"WebHook", "https://deviceaccess.ru/tg_bot"},
        Helpz::Param<uint16_t>{"WebHookPort", 8033},
        Helpz::Param<std::string>{"WebHookCert", std::string()},
        Helpz::Param<std::string>{"TemplatesPath", std::string()}
        ).ptr<Bot::Controller>();
    bot_->start();
}

void Worker::init_informer(QSettings* s)
{
    informer_ = Helpz::SettingsHelper(
        s, "Informer",
        Helpz::Param<bool>{"SkipConnectedEvent", false},
        Helpz::Param<int>{"EventTimeoutSecons", 10 * 60}
        ).ptr<Informer>();

    using namespace boost::placeholders;
    informer_->send_message_signal_.connect(boost::bind(&Bot::Controller::send_message, bot_, _1, _2));
}

void Worker::init_dbus_interface(QSettings* s)
{
    dbus_handler_ = new Dbus_Handler(this);
    dbus_ = Helpz::SettingsHelper(
                s, "DBus_Interface", dbus_handler_,
                Helpz::Param{"Service", DAS_DBUS_DEFAULT_SERVICE_SERVER},
                Helpz::Param{"Object", DAS_DBUS_DEFAULT_OBJECT},
                Helpz::Param{"Interface", DAS_DBUS_DEFAULT_INTERFACE}
                ).ptr<DBus::Interface>();
}

void Worker::processCommands(const QStringList &args)
{
    QList<QCommandLineOption> opt{
        { { "t", "send_by_time" }, QCoreApplication::translate("main", "Send message by time."), "time_secs" },
    };

    QCommandLineParser parser;
    parser.setApplicationDescription("Das service");
    parser.addHelpOption();
    parser.addOptions(opt);

    parser.process(args);

    if (opt.size() > 0 && parser.isSet(opt.at(0)))
    {
        // TODO.
    }
    else if (false) {

    }
    else
        qCInfo(Service::Log) << args << parser.helpText();
}

} // namespace Das
