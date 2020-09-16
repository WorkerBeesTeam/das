#ifndef DAS_SERVER_WORKER_H
#define DAS_SERVER_WORKER_H

#include <future>

#include <QTimer>

//#include <Helpz/settingshelper.h>
//#include <Helpz/db_thread.h>

#include <plus/das/scheme_info.h>

#include "command_line_parser.h"
//#include "server.h"
//#include "work_object.h"

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace Helpz {
namespace DB {
class Connection_Info;
} // namespace DB
namespace DTLS {
class Server_Thread;
class Server_Node;
} // namespace DTLS
} // namespace Helpz

namespace Das {

namespace DB {
class Thread_Manager;
} // namespace DB

namespace Server {

namespace Log_Saver {
class Controller;
} // namespace Log_Saver

//class Informer;
class Dbus_Object;

class Worker : public QObject
{
    Q_OBJECT

public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

    std::shared_ptr<Helpz::DTLS::Server_Node> find_client(uint32_t scheme_id) const;
    std::future<std::shared_ptr<Helpz::DTLS::Server_Node>> find_client_future(uint32_t scheme_id);

    void save_connection_state_to_log(uint32_t scheme_id, const std::chrono::system_clock::time_point &time_point, bool state);

private:
    void init_logging(QSettings* s);
public:
signals:
    void processCommands(const QStringList &args);
public slots:
private slots:
    void on_timer();
private:

    void init_database(QSettings *s);
    void init_log_saver(QSettings* s);
    void init_server(QSettings *s);
    void init_dbus(QSettings* s);

    std::chrono::seconds disconnect_event_timeout_;

    Command_Line_Parser cl_parser_;

    QTimer timer_;

public:
    Helpz::DB::Connection_Info* db_conn_info_;

    DB::Thread_Manager* db_thread_mng_;

    Helpz::DTLS::Server_Thread* server_thread_;

    struct Recently_Connected
    {
        struct Recent_Client
        {
            Scheme_Info scheme_;
            std::chrono::system_clock::time_point disconnect_time_;

            bool operator ==(const Scheme_Info& scheme) const
            {
                return scheme_ == scheme;
            }
        };
        std::vector<Recent_Client> scheme_id_vect_;
        std::mutex mutex_;

        bool remove(const Scheme_Info& scheme)
        {
            std::lock_guard lock(mutex_);
            auto it = std::find(scheme_id_vect_.begin(), scheme_id_vect_.end(), scheme);
            if (it != scheme_id_vect_.end())
            {
                scheme_id_vect_.erase(it);
                return true;
            }
            return false;
        }

        void disconnected(const Scheme_Info& scheme)
        {
            std::lock_guard lock(mutex_);
            scheme_id_vect_.push_back({ scheme, std::chrono::system_clock::now() });
        }

    } recently_connected_;

    Dbus_Object* dbus_;

    Log_Saver::Controller* _log_saver;
};

} // namespace Server
} // namespace Das

#endif // DAS_SERVER_WORKER_H
