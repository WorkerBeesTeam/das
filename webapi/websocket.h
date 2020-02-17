#ifndef DAS_NETWORK_WEBSOCKET_H
#define DAS_NETWORK_WEBSOCKET_H

#include <set>

#include <QtWebSockets/QWebSocketServer>
#include <QLoggingCategory>
#include <QSslError>
#include <QJsonValue>
#include <QMutex>

#include <Das/device_item.h>
#include <Das/param/paramgroup.h>
#include <Das/db/dig_param_value.h>
#include <Das/log/log_pack.h>

#include <plus/das/scheme_info.h>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

namespace Das {

class JWT_Helper;

Q_DECLARE_LOGGING_CATEGORY(WebSockLog)

namespace Network {

struct Websocket_Client
{
    Websocket_Client();
//        WebClient(QWebSocket *socket, uint32_t id = 0) : sock(socket), scheme_id(id) {}
//    bool is_authenticated_;
//        bool isStaff = false;

    uint32_t id_ = 0, last_scheme_id_queried_ = 0;
//        uint32_t group_id = 0;
    std::set<uint32_t> scheme_group_id_set_;
    mutable qint64 auth_sended_time_ = 0;

//        std::vector<uint32_t> accepted;
//        bool operator ==(const WebClient& other) const { return sock == other.sock; }
};

class WebSocket : public QObject
{
    Q_OBJECT
public:
    explicit WebSocket(std::shared_ptr<JWT_Helper> jwt_helper, quint16 port, const QString &certFilePath = QString(), const QString &keyFilePath = QString(),
                       QObject *parent = nullptr);
    ~WebSocket();

    QByteArray prepare_connection_state_message(uint32_t scheme_id, uint8_t connection_state) const;

signals:
    void closed();

    void through_command(std::shared_ptr<Network::Websocket_Client> client, uint32_t scheme_id, quint8 cmd, const QByteArray& data);

public slots:
    void sendDevice_ItemValues(const Scheme_Info& scheme, const QVector<Log_Value_Item>& pack);
    void send_dig_mode_pack(const Scheme_Info& scheme, const QVector<DIG_Mode>& pack);
    void sendModeChanged(const Scheme_Info& scheme, const DIG_Mode &mode);
    void send_dig_param_values_changed(const Scheme_Info& scheme, const QVector<DIG_Param_Value> &pack);
    void send_connection_state(const Scheme_Info& scheme, uint8_t connection_state);
    void sendEventMessage(const Scheme_Info& scheme, const QVector<Log_Event_Item>& event_pack);

    void sendStatusInserted(const Scheme_Info& scheme, uint32_t group_id, uint32_t info_id, const QStringList& args);
    void sendStatusRemoved(const Scheme_Info& scheme, uint32_t group_id, uint32_t info_id);
    void send_structure_changed(const Scheme_Info& scheme, const QByteArray& data);
    void send_time_info(const Scheme_Info& scheme, const QTimeZone& tz, qint64 time_offset);
    void send_ip_address(const Scheme_Info& scheme, const QString& ip_address);

    void send(const Scheme_Info &scheme, const QByteArray& data) const;
    void send_to_client(std::shared_ptr<Network::Websocket_Client> client, const QByteArray& data) const;
private slots:
    void originAuthentication(QWebSocketCorsAuthenticator *pAuthenticator);
    void acceptError(QAbstractSocket::SocketError socketError);
    void serverError(QWebSocketProtocol::CloseCode closeCode);
    void onNewConnection();
    void processBinaryMessage(const QByteArray &message);
    void onSslErrors(const QList<QSslError> &errors);
    void socketDisconnected();

private:
    bool auth(const QByteArray& token, Websocket_Client& client);

    QWebSocketServer *server_;

    QMap<QWebSocket*, std::shared_ptr<Websocket_Client>> client_map_;
    mutable QMutex clients_mutex_;

    std::shared_ptr<JWT_Helper> jwt_helper_;
};

} // namespace Network
} // namespace Das

#endif // DAS_NETWORK_WEBSOCKET_H
