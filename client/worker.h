#ifndef DAS_CLIENT_WORKER_H
#define DAS_CLIENT_WORKER_H

#include <qglobal.h>

#ifdef QT_DEBUG
#include <QApplication>
typedef QApplication App_Type;
#else
#include <QGuiApplication>
typedef QGuiApplication App_Type;
#endif

#include <Helpz/service.h>
#include <Helpz/settingshelper.h>

#include <Helpz/dtls_client_thread.h>

#include <plus/das/database.h>
#include <plus/das/scheme_info.h>

#include "checker_manager.h"
#include "Network/client_protocol_latest.h"
#include "Scripts/scripted_scheme.h"

#include "worker_structure_synchronizer.h"
#include "log_value_save_timer.h"

namespace Das {

namespace Client {
    class Dbus_Object;
}

class Worker final : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = 0);
    ~Worker();

    const Scheme_Info& scheme_info() const;

    Helpz::DB::Thread* db_pending();

    static std::unique_ptr<QSettings> settings();

    Scripted_Scheme* prj();

    Worker_Structure_Synchronizer* structure_sync();

    std::shared_ptr<Ver::Client::Protocol> net_protocol();

    void close_net_client();

    QByteArray start_stream(uint32_t user_id, uint32_t dev_item_id, const QString &url);

    static void store_connection_id(const QUuid& connection_id);
    Client::Dbus_Object *dbus() const;

private:
    void init_logging(QSettings* s);
    void init_dbus(QSettings* s);
    void init_database(QSettings *s);
    void init_scheme(QSettings* s);
    void init_checker(QSettings* s);
    void init_network_client(QSettings* s);
    void init_log_timer();
signals:
    void serviceRestart();

    // D-BUS Signals
    void started();
//    void change(const Log_Value_Item& item, bool immediately);

    void mode_changed(uint32_t user_id, uint32_t mode_id, uint32_t group_id);
    void status_changed(const DIG_Status& status);
public slots:
    void restart_service_object(uint32_t user_id = 0);
    bool stop_scripts(uint32_t user_id = 0);

    void logMessage(QtMsgType type, const Helpz::LogContext &ctx, const QString &str);
    void add_event_message(Log_Event_Item event);
    void processCommands(const QStringList& args);

    QString get_user_devices();
    QString get_user_status();

    void init_device(const QString& device, const QString& device_name, const QString &device_latin, const QString& device_desc);
    void clear_server_config();
    void save_server_auth_data(const QString& login, const QString& password);
    void save_server_data(const QUuid &devive_uuid, const QString& login, const QString& password);

    void set_mode(uint32_t user_id, uint32_t mode_id, uint32_t group_id);
    void set_scheme_name(uint32_t user_id, const QString& name);

    void update_plugin_param_names(const QVector<Plugin_Type>& plugins);
public slots:
    void connection_state_changed(Device_Item *item, bool value);
private:
    friend class Ver::Client::Protocol;
    friend class Client::Protocol_Base;
    std::shared_ptr<Helpz::DTLS::Client_Thread> net_thread_;
    std::unique_ptr<Helpz::DB::Thread> db_pending_thread_;
    Worker_Structure_Synchronizer* structure_sync_;

    using Scripts_Thread = Helpz::ParamThread<Scripted_Scheme, Worker*, Helpz::ConsoleReader*, QString, bool, bool>;
    Scripts_Thread* scheme_thread_;
    Scripted_Scheme* prj_;

    friend class Checker::Manager;
    using Checker_Thread = Helpz::SettingsThreadHelper<Checker::Manager, Worker*/*, QStringList*/>;
    Checker_Thread::Type* checker_th_;

    using Log_Value_Save_Timer_Thread = Helpz::ParamThread<Log_Value_Save_Timer, Worker*>;
    Log_Value_Save_Timer_Thread* log_timer_thread_;
    friend class Scripted_Scheme;

    uint32_t restart_user_id_;
    QTimer restart_timer_;

    Client::Dbus_Object* dbus_;
    Scheme_Info scheme_info_;
};

typedef Helpz::Service::Impl<Worker, App_Type> Service;

} // namespace Das

#endif // DAS_CLIENT_WORKER_H
