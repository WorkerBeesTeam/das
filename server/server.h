#ifndef DAS_SERVER_H
#define DAS_SERVER_H

#include <Helpz/db_connection_info.h>

/*
#include <memory>

#include <boost/asio.hpp>

#include <Helpz/dtls_server_thread.h>
#include <QString>
//#include <Helpz/settingshelper.h>

//#include "websocket.h"

*/

namespace Das {
/*
namespace DB {
class global;
}

//typedef std::shared_ptr<das::database::global> (*DatabaseOpenFunc)(const std::string&);

struct AppGlobal {
//    Net::WebSocket* webSock;
    //using WebSocketThread = Helpz::SettingsThreadHelper<Net::WebSocket, QString, QString, quint16>;
    //extern WebSocketThread::Type* webSock_th;

//    dtls::server* dtls_server;
//    dtls::tools* dtls_tools;

//    DatabaseOpenFunc open_database;
    //extern database::ConnectionInfo* db_conn_info;

//    server_config* serv_conf;
};

extern Net::WebSocket* webSock;
extern Helpz::DTLS::Server_Thread* server_thread;
*/

extern Helpz::DB::Connection_Info* db_conn_info;

} // namespace Das

#endif // DAS_SERVER_H
