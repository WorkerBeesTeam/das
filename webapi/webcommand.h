#ifndef DAS_WEBCOMMAND_H
#define DAS_WEBCOMMAND_H

#include <Helpz/dtls_server_node.h>

#include "websocket.h"

namespace Das {
namespace Net {

class WebCommand : public QObject
{
    Q_OBJECT
public:
    WebCommand(WebSocket* webSock);
signals:
    void send(std::shared_ptr<Websocket_Client> client, const QByteArray& data) const;

    uint8_t get_scheme_connection_state(const std::set<uint32_t> &scheme_group_set, uint32_t scheme_id);
    uint8_t get_scheme_connection_state2(uint32_t scheme_id);
    void send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& data);
public slots:
private slots:
    void write_command(std::shared_ptr<Net::Websocket_Client> client, uint32_t scheme_id, quint8 cmd, const QByteArray& data);
private:
    WebSocket* websock_;
};

} // namespace Net
} // namespace Das

#endif // DAS_WEBCOMMAND_H
