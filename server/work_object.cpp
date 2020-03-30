#include <future>

#include <botan/parsing.h>

#include <QCoreApplication>

#include <Helpz/db_connection_info.h>
#include <Helpz/dtls_server_thread.h>

#include "database/db_thread_manager.h"
#include "server_protocol.h"
#include "dbus_object.h"

#include "work_object.h"

namespace Das {

Helpz::DB::Connection_Info* db_conn_info;

namespace Server {

Work_Object::Work_Object() :
    db_conn_info_(nullptr),
    db_thread_mng_(nullptr),
    server_thread_(nullptr),
    dbus_(nullptr)
{
}

Work_Object::~Work_Object()
{
    delete server_thread_;

    delete db_thread_mng_;
    delete db_conn_info_; db_conn_info = nullptr;
    delete dbus_;
}

std::shared_ptr<Helpz::DTLS::Server_Node> Work_Object::find_client(uint32_t scheme_id) const
{
    return dbus_->find_client(scheme_id);
}

std::future<std::shared_ptr<Helpz::DTLS::Server_Node>> Work_Object::find_client_future(uint32_t scheme_id)
{
    auto promise = std::make_shared<std::promise<std::shared_ptr<Helpz::DTLS::Server_Node>>>();
    auto future = promise->get_future();

    server_thread_->io_context()->post([=]()
    {
        promise->set_value(dbus_->find_client(scheme_id));
    });

    return future;
}

void Work_Object::save_connection_state_to_log(uint32_t scheme_id, const std::chrono::system_clock::time_point &time_point, bool state)
{
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch());

    Log_Event_Item item{msec.count(), 0, false,
                state ? Log_Event_Item::ET_INFO : Log_Event_Item::ET_WARNING, "server",
                (state ? QString() : "DIS") + "CONNECTED"};
    item.set_scheme_id(scheme_id);

    auto values = Log_Event_Item::to_variantlist(item);

    db_thread_mng_->log_thread()->add([values](Helpz::DB::Base* db)
    {
        db->insert(Helpz::DB::db_table<Log_Event_Item>(), values);
    });
}

void Work_Object::init_database(QSettings* s)
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
    db_conn_info = db_conn_info_;
    Helpz::DB::Connection_Info::set_common(*db_conn_info_);

    db_thread_mng_ = new DB::Thread_Manager{*db_conn_info_};
}

void Work_Object::init_server(QSettings* s)
{
    Helpz::DTLS::Create_Server_Protocol_Func_T create_protocol = [this](const std::vector<std::string> &client_protos, std::string* choose_out) -> std::shared_ptr<Helpz::Net::Protocol>
    {
        for (const std::string& proto: client_protos)
        {
            const std::vector<std::string> proto_arr = Botan::split_on(proto, '/');
            if (proto_arr.size() != 2 || proto_arr.front() != "das")
                continue;

            const std::string& ver_str = proto_arr.back();

            if (ver_str == "2.5")
            {
                *choose_out = proto;
                return std::make_shared<Ver::Server::Protocol>(this);
            }
            else if (ver_str == "2.4")
            {
                *choose_out = proto;
            }
            else if (true) {}
            else if (ver_str == "2.1" && (choose_out->empty() || ver_str == "2.0"))
            {
                *choose_out = proto;
            }
            else if (choose_out->empty() && ver_str == "2.0")
            {
                *choose_out = proto;
            }
        }

        if (*choose_out == "das/2.4")
        {
            auto ptr = std::make_shared<Ver::Server::Protocol>(this);
            ptr->disable_sync();
            return ptr;
        }
        else if (*choose_out == "das/2.0")
        {
//            return std::make_shared<Ver_2_0::Server::Protocol>(this);
        }

        std::cerr << "Unsuported protocol:";
        for (const std::string& proto: client_protos)
            std::cerr << ' ' << proto;
        std::cerr << std::endl;
        return nullptr;
    };

    const QString default_dir = qApp->applicationDirPath() + '/';
    auto [ port, tls_policy_file, certificate_file, certificate_key_file,
            cleaning_timeout, receive_thread_count, record_thread_count,
            disconnect_event_timeout ]
            = Helpz::SettingsHelper{
        s, "Server",
                Helpz::Param{"Port", (uint16_t)25588},
                Helpz::Param{"TlsPolicyFile", default_dir + "tls_policy.conf"},
                Helpz::Param{"CertificateFile", default_dir + "dtls.pem"},
                Helpz::Param{"CertificateKeyFile", default_dir + "dtls.key"},
                Helpz::Param{"CleaningTimeout", (uint32_t)3 * 60},
                Helpz::Param{"ReceiveThreadCount", (uint16_t)5},
                Helpz::Param{"RecordThreadCount", (uint16_t)5},
                Helpz::Param{"DisconnectEventTimeoutSeconds", 60}
    }();

    disconnect_event_timeout_ = std::chrono::seconds{disconnect_event_timeout};

    Helpz::DTLS::Server_Thread_Config conf{ port, tls_policy_file.toStdString(), certificate_file.toStdString(), certificate_key_file.toStdString(),
                cleaning_timeout, receive_thread_count, record_thread_count, 60 };

    conf.set_create_protocol_func(std::move(create_protocol));
    server_thread_ = new Helpz::DTLS::Server_Thread{std::move(conf)};
}

void Work_Object::init_dbus(QSettings* s)
{
    dbus_ = Helpz::SettingsHelper(
                s, "DBus", server_thread_->server(),
                Helpz::Param{"Service", DAS_DBUS_DEFAULT_SERVICE_SERVER},
                Helpz::Param{"Object", DAS_DBUS_DEFAULT_OBJECT}
                ).ptr<Dbus_Object>();
}

} // namespace Server
} // namespace Das
