#include <future>

#include <QCoreApplication>

#include <Helpz/db_connection_info.h>
#include <Helpz/dtls_server_thread.h>

#include "database/db_thread_manager.h"
#include "old/server_protocol_2_0.h"
#include "old/server_protocol_2_1.h"
#include "server_protocol.h"
#include "dbus_object.h"

#include "work_object.h"

namespace Das {

Helpz::Database::Connection_Info* db_conn_info;

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

void Work_Object::init_database(QSettings* s)
{
    db_conn_info_ = Helpz::SettingsHelper(
                s, "Database",
                Helpz::Param{"Name", "das"},
                Helpz::Param{"User", "DasUser"},
                Helpz::Param{"Password", QString()},
                Helpz::Param{"Host", "localhost"},
                Helpz::Param{"Port", 3306},
                Helpz::Param{"Prefix", "das_"},
                Helpz::Param{"Driver", "QMYSQL"}, // QPSQL
                Helpz::Param{"ConnectOptions", QString()}
                ).ptr<Helpz::Database::Connection_Info>();
    db_conn_info = db_conn_info_;
    Helpz::Database::Connection_Info::set_common(*db_conn_info_);

    db_thread_mng_ = new Database::Thread_Manager{*db_conn_info_};
}

void Work_Object::init_server(QSettings* s)
{
    Helpz::DTLS::Create_Server_Protocol_Func_T create_protocol = [this](const std::vector<std::string> &client_protos, std::string* choose_out) -> std::shared_ptr<Helpz::Network::Protocol>
    {
        for (const std::string& proto: client_protos)
        {
            if (proto == "das/2.4")
            {
                *choose_out = proto;
                return std::make_shared<Ver_2_4::Server::Protocol>(this);
            }
            else if (true) {}
            else if (proto == "das/2.1" && (choose_out->empty() || proto == "das/2.0"))
            {
                *choose_out = proto;
            }
            else if (choose_out->empty() && proto == "das/2.0")
            {
                *choose_out = proto;
            }
        }

        if (*choose_out == "das/2.1")
        {
//            return std::make_shared<Ver_2_1::Server::Protocol>(this);
        }
        else if (*choose_out == "das/2.0")
        {
//            return std::make_shared<Ver_2_0::Server::Protocol>(this);
        }

        std::cerr << "Unsuported protocol" << std::endl;
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
