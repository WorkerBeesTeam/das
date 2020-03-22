#ifndef DAS_SERVER_WEBAPI_WORKER_H
#define DAS_SERVER_WEBAPI_WORKER_H

#include <QTimer>

#include <Helpz/service.h>
#include <Helpz/settingshelper.h>
#include <Helpz/db_thread.h>

#include "webcommand.h"
#include "stream/stream_server_thread.h"

namespace Das {

namespace Rest {
class Restful;
} // namespace Rest

namespace DBus {
class Interface;
} // namespace DBus

namespace Server {
namespace WebApi {

class Dbus_Handler;

class Worker : public QObject
{
    Q_OBJECT

public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

private:
    void init_logging(QSettings* s);
    void init_database(QSettings* s);
    void init_dbus_interface(QSettings* s);
    void init_jwt_helper(QSettings* s);
    void init_websocket_manager(QSettings *s);
    void init_web_command(QSettings* s);
    void init_restful(QSettings *s);
    void init_stream_server(QSettings *s);
signals:
private slots:
private:
    Helpz::DB::Connection_Info* db_conn_info_;
    Helpz::DB::Thread* db_pending_thread_;

    Dbus_Handler* dbus_handler_;
    DBus::Interface* dbus_;
    friend class Dbus_Handler;

    using Websocket_Thread = Helpz::SettingsThreadHelper<Net::WebSocket, std::shared_ptr<JWT_Helper>, QString, quint16, QString, QString>;
    Websocket_Thread::Type* websock_th_;

    using WebCommandThread = Helpz::ParamThread<Net::WebCommand, Net::WebSocket*>;
    WebCommandThread* web_command_th_;

    Rest::Restful* restful_;

    std::shared_ptr<JWT_Helper> jwt_helper_;

    Stream_Server_Thread* stream_server_;
};

typedef Helpz::Service::Impl<Worker> Service;

} // namespace WebApi
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_WEBAPI_WORKER_H
