#ifndef DAS_SERVER_WEBAPI_WORKER_H
#define DAS_SERVER_WEBAPI_WORKER_H

#include <QTimer>

#include <Helpz/service.h>
#include <Helpz/settingshelper.h>
#include <Helpz/db_thread.h>

namespace Das {
namespace DBus {
class Interface;
} // namespace DBus
} // namespace Das

namespace Das {

class SMTP_Config;

using namespace Das;

namespace Bot {
class Controller;
} // namespace Bot

class Dbus_Handler;

class Informer;

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

private:
    void init_logging(QSettings* s);
    void init_database(QSettings* s);
    void init_bot(QSettings* s);
    void init_informer(QSettings* s);
    void init_dbus_interface(QSettings* s);

signals:
public slots:
    void processCommands(const QStringList& args);
private slots:
private:
    Helpz::DB::Connection_Info* db_conn_info_;
    Helpz::DB::Thread* db_pending_thread_;

    Bot::Controller* bot_;

    Dbus_Handler* dbus_handler_;
    DBus::Interface* dbus_;
    friend class Dbus_Handler;

    Informer* informer_;

    friend class Dbus_Interface;
};

typedef Helpz::Service::Impl<Worker> Service;

} // namespace Das

#endif // DAS_SERVER_WEBAPI_WORKER_H
