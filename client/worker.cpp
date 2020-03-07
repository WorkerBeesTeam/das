#include <QDebug>
#include <QDir>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTimer>

#include <botan-2/botan/parsing.h>

#include <Helpz/consolereader.h>
#include <Helpz/dtls_client.h>

#include <Das/commands.h>
#include <Das/checker_interface.h>
#include <Das/device.h>
#include <Das/db/dig_status.h>
#include <Das/db/dig_mode.h>

#include "dbus_object.h"
#include "worker.h"

namespace Das {

namespace Z = Helpz;
using namespace std::placeholders;

// ---------------------------------------------------------------------------------

Worker::Worker(QObject *parent) :
    QObject(parent),
    structure_sync_(nullptr),
    scheme_thread_(nullptr), prj_(nullptr),
    checker_th_(nullptr),
    log_timer_thread_(nullptr),
    restart_user_id_(0),
    dbus_(nullptr),
    scheme_info_(1, {1})
{
    qsrand(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    qRegisterMetaType<uint32_t>("uint32_t");

    auto s = settings();

    init_logging(s.get());
    init_dbus(s.get());
    init_database(s.get());
    init_scheme(s.get()); // инициализация структуры проекта
    init_log_timer(); // сохранение статуса устройства по таймеру
    init_checker(s.get()); // запуск потока опроса устройств
    init_network_client(s.get()); // подключение к серверу

    restart_timer_.setSingleShot(true);
    restart_timer_.setInterval(3000);
    connect(&restart_timer_, SIGNAL(timeout()), this, SLOT(restart_service_object()));

    emit started();

//    QTimer::singleShot(15000, thread(), SLOT(quit())); // check delete error
}

template<typename T>
void stop_thread(T** thread_ptr, unsigned long wait_time = 15000)
{
    if (*thread_ptr)
    {
        QThread* thread = *thread_ptr;
        thread->quit();
        if (!thread->wait(wait_time))
        {
            thread->terminate();
        }
        delete *thread_ptr;
        *thread_ptr = nullptr;
    }
}

Worker::~Worker()
{
    blockSignals(true);
    QObject::disconnect(this, 0, 0, 0);

    stop_thread(&log_timer_thread_, 30000);

    net_thread_.reset();
    stop_thread(&checker_th_);
    if (!scheme_thread_ && prj_)
    {
        delete prj_;
    }
    else
    {
        stop_thread(&scheme_thread_);
    }

    if (structure_sync_)
    {
        delete structure_sync_;
    }

    db_pending_thread_.reset();
    delete dbus_;
}

const Scheme_Info &Worker::scheme_info() const
{
    return scheme_info_;
}

Helpz::DB::Thread* Worker::db_pending() { return db_pending_thread_.get(); }

/*static*/ std::unique_ptr<QSettings> Worker::settings()
{
    QString configFileName = QCoreApplication::applicationDirPath() + QDir::separator() + QCoreApplication::applicationName() + ".conf";
    return std::unique_ptr<QSettings>(new QSettings(configFileName, QSettings::NativeFormat));
}

Scripted_Scheme* Worker::prj()
{
    return scheme_thread_ ? scheme_thread_->ptr() : prj_;
}

Worker_Structure_Synchronizer* Worker::structure_sync()
{
    return structure_sync_;
}

std::shared_ptr<Ver::Client::Protocol> Worker::net_protocol()
{
    if (net_thread_)
    {
        auto client = net_thread_->client();
        if (client)
            return std::static_pointer_cast<Ver::Client::Protocol>(client->protocol());
    }
    return {};
}

/*static*/ void Worker::store_connection_id(const QUuid &connection_id)
{
    std::shared_ptr<QSettings> s = settings();
    s->beginGroup("RemoteServer");
    s->setValue("Device", connection_id);
    s->endGroup();
}

Client::Dbus_Object *Worker::dbus() const
{
    return dbus_;
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

void Worker::init_dbus(QSettings* s)
{
    dbus_ = Helpz::SettingsHelper(s, "DBus", this,
                Helpz::Param{"Service", DAS_DBUS_DEFAULT_SERVICE_CLIENT},
                Helpz::Param{"Object", DAS_DBUS_DEFAULT_OBJECT}
                ).ptr<Client::Dbus_Object>();
}

void Worker::init_database(QSettings* s)
{
    const char db_conf_group_name[] = "Database";

    auto [scheme_id] = Helpz::SettingsHelper(
                s, db_conf_group_name,
                Z::Param<uint32_t>{"SchemeId", 1})();

    DB::Schemed_Model::set_default_scheme_id(scheme_id);

    auto db_info = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Z::Param<QString>,Z::Param<QString>,Z::Param<QString>,Z::Param<QString>,Z::Param<QString>,Z::Param<int>,Z::Param<QString>,Z::Param<QString>>
        #endif
            (
                s, db_conf_group_name,
                Z::Param<QString>{"Name", "das"},
                Z::Param<QString>{"User", "das"},
                Z::Param<QString>{"Password", ""},
                Z::Param<QString>{"Host", "localhost"},
                Z::Param<int>{"Port", -1},
                Z::Param<QString>{"Prefix", "das_"},
                Z::Param<QString>{"Driver", "QMYSQL"},
                Z::Param<QString>{"ConnectOptions", "CLIENT_FOUND_ROWS=1;MYSQL_OPT_RECONNECT=1"}
    ).obj<Helpz::DB::Connection_Info>();

    Helpz::DB::Connection_Info::set_common(db_info);

    db_pending_thread_.reset(new Helpz::DB::Thread);
}

void Worker::init_scheme(QSettings* s)
{
    qRegisterMetaType<QVector<DIG_Status>>("QVector<DIG_Status>");

    Helpz::ConsoleReader* cr = nullptr;
    if (Service::instance().isImmediately())
    {
        if (qApp->arguments().indexOf("-disable-console") == -1)
            cr = new Helpz::ConsoleReader(this);

#ifdef QT_DEBUG
        if (qApp->arguments().indexOf("-debugger") != -1)
        {
            qApp->setQuitOnLastWindowClosed(false);

            auto server_conf = Helpz::SettingsHelper{
                                s, "Server",
                                Z::Param<QString>{"SSHHost", "80.89.129.98"},
                                Z::Param<bool>{"AllowShell", false},
                                Z::Param<bool>{"OnlyFromFolderIfExist", false}
                            }();
            prj_ = new Scripted_Scheme(this, cr, std::get<0>(server_conf), std::get<1>(server_conf));
        }
#endif
    }

    if (!prj_)
    {
        scheme_thread_ = Scripts_Thread()(s, "Server", this,
                              cr,
                              Z::Param<QString>{"SSHHost", "80.89.129.98"},
                              Z::Param<bool>{"AllowShell", false},
                              Z::Param<bool>{"OnlyFromFolderIfExist", false}
                              );
        scheme_thread_->start(QThread::HighPriority);
    }
}

void Worker::init_checker(QSettings* s)
{
    qRegisterMetaType<Device*>("Device*");

    checker_th_ = Checker_Thread()(s, "Checker", this, Z::Param<QStringList>{"Plugins", QStringList{"ModbusPlugin","WiringPiPlugin"}} );
    checker_th_->start();
}

void Worker::init_network_client(QSettings* s)
{
    structure_sync_ = new Worker_Structure_Synchronizer{ this };

    qRegisterMetaType<Log_Event_Item>("Log_Event_Item");
    qRegisterMetaType<QVector<Log_Value_Item>>("QVector<Log_Value_Item>");
    qRegisterMetaType<QVector<Log_Event_Item>>("QVector<Log_Event_Item>");
    qRegisterMetaType<QVector<uint32_t>>("QVector<uint32_t>");

    Authentication_Info auth_info = Helpz::SettingsHelper{
                s, "RemoteServer",
                Z::Param<QString>{"Login",              QString()},
                Z::Param<QString>{"Password",           QString()},
                Z::Param<QString>{"SchemeName",        QString()},
                Z::Param<QUuid>{"Device",               QUuid()},
            }.obj<Authentication_Info>();

    if (!auth_info)
    {
        return;
    }

#define DAS_PROTOCOL_LATEST "das/2.4"
#define DAS_PROTOCOL_SUPORTED DAS_PROTOCOL_LATEST",das/2.3"

    const QString default_dir = qApp->applicationDirPath() + '/';
    auto [ tls_policy_file, host, port, protocols, recpnnect_interval_sec ]
            = Helpz::SettingsHelper{
                s, "RemoteServer",
                Z::Param<QString>{"TlsPolicyFile", default_dir + "tls_policy.conf"},
                Z::Param<QString>{"Host",               "deviceaccess.ru"},
                Z::Param<QString>{"Port",               "25588"},
                DAS_PROTOCOL_SUPORTED, // Z::Param<QString>{"Protocols",          "das/2.0,das/1.1"},
                Z::Param<uint32_t>{"ReconnectSeconds",       15}
            }();

    Helpz::DTLS::Create_Client_Protocol_Func_T func = [this, auth_info](const std::string& app_protocol) -> std::shared_ptr<Helpz::Network::Protocol>
    {
        std::shared_ptr<Ver::Client::Protocol> ptr = std::make_shared<Ver::Client::Protocol>(this, auth_info);

        if (app_protocol != DAS_PROTOCOL_LATEST)
        {
            qCWarning(Service::Log) << "Server doesn't support protocol:" << DAS_PROTOCOL_LATEST << "server want:" << app_protocol.c_str();

            std::vector<std::string> proto_arr = Botan::split_on(app_protocol, '/');
            if (proto_arr.size() == 2)
            {
                if (std::atof(proto_arr.back().c_str()) <= 2.2)
                {
//                    ptr->use_repeated_flag_ = false;
                }
            }
        }

        return std::static_pointer_cast<Helpz::Network::Protocol>(ptr);
    };

    Helpz::DTLS::Client_Thread_Config conf{ tls_policy_file.toStdString(), host.toStdString(), port.toStdString(),
                Botan::split_on(protocols.toStdString(), ','), recpnnect_interval_sec };
    conf.set_create_protocol_func(std::move(func));

    net_thread_.reset(new Helpz::DTLS::Client_Thread{std::move(conf)});
}

void Worker::init_log_timer()
{
    qRegisterMetaType<QVector<Device_Item_Value>>("QVector<Device_Item_Value>");

    log_timer_thread_ = new Log_Value_Save_Timer_Thread(this);
    log_timer_thread_->start();
}

void Worker::restart_service_object(uint32_t user_id)
{
    if (restart_timer_.isActive())
    {
        Log_Event_Item event { QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), user_id, false, QtDebugMsg, Service::Log().categoryName(),
                    "The service restart is already runing. Remaining time: " + QString::number(restart_timer_.remainingTime())};
        add_event_message(std::move(event));
        return;
    }

    if (user_id == 0)
        user_id = restart_user_id_;
    else
        restart_user_id_ = user_id;

    if (prj() && !stop_scripts(user_id))
    {
        restart_timer_.start();
        return;
    }

    Log_Event_Item event { QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), user_id, false, QtInfoMsg, Service::Log().categoryName(), "The service restarts."};
    add_event_message(std::move(event));
    QTimer::singleShot(50, this, SIGNAL(serviceRestart()));
}

bool Worker::stop_scripts(uint32_t user_id)
{
    bool is_stoped = true;
#ifdef QT_DEBUG
    if (prj()->thread() == thread())
    {
        is_stoped = prj()->can_restart(is_stoped, user_id);
    }
    else
#endif
    QMetaObject::invokeMethod(prj(), "can_restart", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, is_stoped), Q_ARG(bool, is_stoped), Q_ARG(uint32_t, user_id));
    return is_stoped;
}

void Worker::logMessage(QtMsgType type, const Helpz::LogContext &ctx, const QString &str)
{
    if (ctx.category().startsWith("net"))
    {
        return;
    }

    Log_Event_Item event{ QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), 0, false, type, ctx.category(), str };

    static QRegularExpression re("^(\\d+)\\|");
    QRegularExpressionMatch match = re.match(str);
    if (match.hasMatch())
    {
        event.set_user_id(match.captured(1).toUInt());
        event.set_text(str.right(str.size() - (match.capturedEnd(1) + 1)));
    }

    add_event_message(std::move(event));
}

void Worker::add_event_message(Log_Event_Item event)
{
    if (log_timer_thread_)
    {
        QMetaObject::invokeMethod(log_timer_thread_->ptr(), "add_log_event_item", Qt::QueuedConnection, Q_ARG(Log_Event_Item, event));
    }
}

void Worker::processCommands(const QStringList &args)
{
    QList<QCommandLineOption> opt{
        { { "c", "console" }, QCoreApplication::translate("main", "Execute command in JS console."), "script" },
        { { "s", "stop_scripts"}, QCoreApplication::translate("main", "Stop scripts")},

        { { "d", "dev", "device" }, QCoreApplication::translate("main", "Device"), "ethX" },

        // A boolean option with a single name (-p)
        {"p",
            QCoreApplication::translate("main", "Show progress during copy")},
        // A boolean option with multiple names (-f, --force)
        {{"f", "force"},
            QCoreApplication::translate("main", "Overwrite existing files.")},
        // An option with a value
        {{"t", "target-directory"},
            QCoreApplication::translate("main", "Copy all source files into <directory>."),
            QCoreApplication::translate("main", "directory")},
    };


    QCommandLineParser parser;
    parser.setApplicationDescription("Das service");
    parser.addHelpOption();
    parser.addOptions(opt);

    parser.process(args);

    if (opt.size() > 0 && parser.isSet(opt.at(0)))
    {
        QMetaObject::invokeMethod(prj(), "console", Qt::QueuedConnection, Q_ARG(uint32_t, 0), Q_ARG(QString, parser.value(opt.at(0))));
    }
    else if (opt.size() > 1 && parser.isSet(opt.at(1)))
    {
        stop_scripts();
    }
    else
        qCInfo(Service::Log) << args << parser.helpText();
}

QString Worker::get_user_devices()
{
    QVector<QPair<QUuid, QString>> devices;

    auto proto = net_protocol();
    if (proto)
    {
//        devices = proto->getUserDevices();
    }

    QJsonArray json;
    QJsonObject obj;
    for (auto&& device: devices)
    {
        obj["device"] = device.first.toString();
        obj["name"] = device.second;
        json.push_back(obj);
    }

    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString Worker::get_user_status()
{
    QJsonObject json;
    auto proto = net_protocol();
    if (proto)
    {
        json["user"] = proto->auth_info().login();
        json["device"] = proto->auth_info().using_key().toString();
    }
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

void Worker::init_device(const QString &device, const QString &device_name, const QString &device_latin, const QString &device_desc)
{
    QUuid devive_uuid(device);
    if (devive_uuid.isNull() && (device_name.isEmpty() || device_latin.isEmpty()))
        return;

    /** Работа:
     * 1. Если есть device
     *   1. Установить, подключиться, получить все данные
     *   2. Отключится
     *   3. Импортировать полученные данные
     * 2. Если есть device_name
     *   1. Подключится с пустым device
     *   2. Отправить команду на создание нового device, подождать и получить UUID в ответе
     *   3. Сохранить UUID
     * 3. Перезагрузится
     **/

    auto proto = net_protocol();
    if (!devive_uuid.isNull() && proto)
    {
//        proto->importDevice(devive_uuid);
    }

    QUuid new_uuid;
    if (!device_name.isEmpty() && proto)
    {
//        new_uuid = proto->createDevice(device_name, device_latin, device_desc);
    }
    else
        new_uuid = devive_uuid;

    auto s = settings();
    s->beginGroup("RemoteServer");
    s->setValue("Device", new_uuid.toString());
    s->endGroup();

    emit serviceRestart();
}

void Worker::clear_server_config()
{
    save_server_data(QUuid(), QString(), QString());
}

void Worker::save_server_auth_data(const QString &login, const QString &password)
{
    auto proto = net_protocol();
    if (proto)
    {
        save_server_data(proto->auth_info().using_key(), login, password);
    }
}

void Worker::save_server_data(const QUuid &devive_uuid, const QString &login, const QString &password)
{
    auto s = settings();
    s->beginGroup("RemoteServer");
    s->setValue("Device", devive_uuid.toString());
    s->setValue("Login", login);
    s->setValue("Password", password);
    s->endGroup();

    auto proto = net_protocol();
    if (proto)
    {
//        proto->refreshAuth();
    }
}

void Worker::set_mode(uint32_t user_id, uint32_t mode_id, uint32_t group_id)
{
    // ATTENTION: This function calls from diffrent threads

    QMetaObject::invokeMethod(prj(), "set_mode", Qt::QueuedConnection, Q_ARG(uint32_t, user_id), Q_ARG(uint32_t, mode_id), Q_ARG(uint32_t, group_id));
}

QVariant db_get_dig_status_id(Helpz::DB::Base* db, const QString& table_name, uint32_t group_id, uint32_t info_id)
{
    auto q = db->select({table_name, {}, {"id"}}, QString("WHERE group_id = %1 AND status_id = %2").arg(group_id).arg(info_id));
    if (q.next())
    {
        return q.value(0);
    }
    return {};
}

void Worker::update_plugin_param_names(const QVector<Plugin_Type>& plugins)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::ReadWrite);
    ds << plugins << uint32_t(0) << uint32_t(0);
    ds.device()->seek(0);
    structure_sync_->process_modify_message(0, Ver::ST_PLUGIN_TYPE, ds.device(),
                                            DB::Schemed_Model::default_scheme_id(), nullptr);

    for (Device* dev: prj()->devices())
    {
        for (const Plugin_Type& plugin: plugins)
        {
            if (plugin.id() == dev->plugin_id())
            {
                dev->set_param_name_list(plugin.param_names_device());
                for (Device_Item *item: dev->items())
                {
                    item->set_param_name_list(plugin.param_names_device_item());
                }
                break;
            }
        }
    }
}

void Worker::connection_state_changed(Device_Item *item, bool value)
{
    Log_Value_Item log_value_item{QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), 0,
                                  item->id(), QVariant(), QVariant(), false};

    if (value)
    {
        log_value_item.set_raw_value(item->raw_value());
        log_value_item.set_value(item->value());
    }

    QMetaObject::invokeMethod(log_timer_thread_->ptr(), "add_log_value_item", Qt::QueuedConnection,
                              Q_ARG(Log_Value_Item, log_value_item));
}

} // namespace Das
