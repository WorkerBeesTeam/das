#include <Das/commands.h>

#include "webcommand.h"

namespace Das {
namespace Network {

WebCommand::WebCommand(WebSocket *webSock) :
    websock_(webSock)
{
    connect(webSock, &WebSocket::through_command, this, &WebCommand::write_command, Qt::QueuedConnection);
    connect(this, &WebCommand::send, webSock, &WebSocket::send_to_client, Qt::QueuedConnection);
}

void WebCommand::write_command(std::shared_ptr<Network::Websocket_Client> client, uint32_t scheme_id, quint8 cmd, const QByteArray &data)
{
//    qDebug() << "writeCommand scheme_group" << user_scheme_group_id << "scheme" << scheme_id << "cmd" << int(cmd);
    uint8_t scheme_connection_state = get_scheme_connection_state(client->scheme_group_id_set_, scheme_id);

    if (cmd == WS_CONNECTION_STATE)
    {
        send(client, websock_->prepare_connection_state_message(scheme_id, scheme_connection_state));
    }

    if (scheme_connection_state <= CS_DISCONNECTED_JUST_NOW)
    {
        return;
    }

    switch (cmd)
    {
    case Das::WS_CONNECTION_STATE:
        break;

    case Das::WS_WRITE_TO_DEV_ITEM:
    case Das::WS_CHANGE_DIG_MODE:
    case Das::WS_CHANGE_DIG_PARAM_VALUES:
    case Das::WS_EXEC_SCRIPT:
    case Das::WS_RESTART:
    case Das::WS_STRUCT_MODIFY:
        send_message_to_scheme(scheme_id, cmd, client->id_, data);
        break;

    default:
        qWarning() << "WebCommand unknown command";
        break;
    }
}

} // namespace Network
} // namespace Das
