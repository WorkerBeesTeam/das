#include <QtWebSockets/QWebSocket>
#include <QWebSocketCorsAuthenticator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QSslKey>
#include <QFile>
#include <QUuid>
#include <QMutexLocker>

#include <Helpz/net_protocol.h>
#include <Helpz/settingshelper.h>

#include <Das/commands.h>
#include <plus/das/jwt_helper.h>

#define PICOJSON_USE_INT64
#include <plus/jwt-cpp/include/jwt-cpp/picojson.h>

#include "websocket.h"

#include <QRegularExpression>
#include <QBitArray>
void func(QDataStream& stream, void* data)
{

#define NS(x) QT_PREPEND_NAMESPACE(x)

    switch (0) {

    case QMetaType::QChar: // 7
        stream >> *static_cast< NS(QChar)*>(data);
        break;

    case QMetaType::QVariantMap: // 8
        stream >> *static_cast< NS(QVariantMap)*>(data);
        break;

    case QMetaType::QStringList: // 11
        stream >> *static_cast< NS(QStringList)*>(data);
        break;

    case QMetaType::QByteArray: // 12
        stream >> *static_cast< NS(QByteArray)*>(data);
        break;

    case QMetaType::QBitArray: // 13
        stream >> *static_cast< NS(QBitArray)*>(data);
        break;

    case QMetaType::QDate: // 14
        stream >> *static_cast< NS(QDate)*>(data);
        break;
    case QMetaType::QTime: // 15
        stream >> *static_cast< NS(QTime)*>(data);
        break;
    case QMetaType::QDateTime: // 16
        stream >> *static_cast< NS(QDateTime)*>(data);
        break;

    case QMetaType::QUrl: // 17
        stream >> *static_cast< NS(QUrl)*>(data);
        break;

    case QMetaType::QRegExp: // 27
        stream >> *static_cast< NS(QRegExp)*>(data);
        break;

    case QMetaType::QVariantHash: // 28
        stream >> *static_cast< NS(QVariantHash)*>(data);
        break;

    case QMetaType::QUuid: // 30
        stream >> *static_cast< NS(QUuid)*>(data);
        break;

    case QMetaType::QRegularExpression: // 44
        stream >> *static_cast< NS(QRegularExpression)*>(data);
        break;

    case QMetaType::QByteArrayList: // 49
        stream >> *static_cast< NS(QByteArrayList)*>(data);
        break;
    }
}

namespace Das {

Q_LOGGING_CATEGORY(WebSockLog, "net.web")

namespace Network {

Websocket_Client::Websocket_Client() :
    id_(0), auth_sended_time_(QDateTime::currentMSecsSinceEpoch())
{
}

// --------------------------------------------------------------------------------

WebSocket::WebSocket(std::shared_ptr<JWT_Helper> jwt_helper, quint16 port, const QString& certFilePath, const QString& keyFilePath, QObject *parent) :
    QObject(parent),
    server_(new QWebSocketServer(QStringLiteral("Device Access Control"),
                                            certFilePath.isEmpty() || keyFilePath.isEmpty() ? QWebSocketServer::NonSecureMode : QWebSocketServer::SecureMode, this)),
    jwt_helper_(std::move(jwt_helper))
{
    if (!certFilePath.isEmpty() && !certFilePath.isEmpty())
    {
        QSslConfiguration sslConfiguration;
        QFile certFile(certFilePath), keyFile(keyFilePath);
        certFile.open(QIODevice::ReadOnly);
        keyFile.open(QIODevice::ReadOnly);
        QSslCertificate certificate(&certFile, QSsl::Pem);
        QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
        certFile.close();
        keyFile.close();
        sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfiguration.setLocalCertificate(certificate);
        sslConfiguration.setPrivateKey(sslKey);
        sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
        server_->setSslConfiguration(sslConfiguration);
    }

    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<Scheme_Info>("Scheme_Info");
    qRegisterMetaType<QTimeZone>("QTimeZone");
    qRegisterMetaType<QVector<DIG_Param_Value>>("QVector<DIG_Param_Value>");
    qRegisterMetaType<std::shared_ptr<Network::Websocket_Client>>("std::shared_ptr<Network::Websocket_Client>");

    connect(server_, &QWebSocketServer::originAuthenticationRequired, this, &WebSocket::originAuthentication);
    connect(server_, &QWebSocketServer::newConnection, this, &WebSocket::onNewConnection);
    connect(server_, &QWebSocketServer::sslErrors, this, &WebSocket::onSslErrors);
    connect(server_, &QWebSocketServer::closed, this, &WebSocket::closed);
    connect(server_, &QWebSocketServer::acceptError, this, &WebSocket::acceptError);
    connect(server_, &QWebSocketServer::serverError, this, &WebSocket::serverError);

//    while ((!worker->n_mng_th && (QThread::msleep(5), true)) ||
//           (!worker->n_mng_th->ptr() && !worker->n_mng_th->wait(5)));
//    connect(worker->n_mng_th->ptr(), &Manager::devItemsChanged,
//            this, &WebSocket::sendDevice_ItemValues, Qt::QueuedConnection);
//    connect(worker->n_mng_th->ptr(), &Manager::connectStateChanged,
//            this, &WebSocket::sendConnectState, Qt::QueuedConnection);
//    connect(worker->n_mng_th->ptr(), &Manager::eventlog,
//            this, &WebSocket::sendEventMessage, Qt::QueuedConnection);
//    connect(worker->n_mng_th->ptr(), &Manager::modeChanged,
//            this, &WebSocket::sendModeChanged, Qt::QueuedConnection);

    if (server_->listen(QHostAddress::Any, port))
        qCDebug(WebSockLog) << "WebSocket listening on port" << port << "url:" << server_->serverUrl().toString();
    else
        qCCritical(WebSockLog) << server_->errorString();
}

WebSocket::~WebSocket()
{
    server_->close();
//    for (auto&& client: m_clients)
//        delete client.sock;
    qDeleteAll(client_map_.keys());
//    qDeleteAll(m_clients);
}

void WebSocket::originAuthentication(QWebSocketCorsAuthenticator *pAuthenticator)
{
    qCritical() << "originAuthenticationRequired" << pAuthenticator->origin()
                << "allowed" << pAuthenticator->allowed();
}

void WebSocket::acceptError(QAbstractSocket::SocketError socketError)
{
    qCritical() << "acceptError" << socketError;
}

void WebSocket::serverError(QWebSocketProtocol::CloseCode closeCode)
{
    qCritical() << "serverError" << closeCode;
}

void WebSocket::onNewConnection()
{
    QWebSocket *socket = server_->nextPendingConnection();
//    socket->moveToThread();
    connect(socket, &QWebSocket::binaryMessageReceived, this, &WebSocket::processBinaryMessage);
    connect(socket, &QWebSocket::disconnected, this, &WebSocket::socketDisconnected);

    QMutexLocker lock(&clients_mutex_);
    client_map_.insert(socket, std::make_shared<Websocket_Client>());
    socket->sendBinaryMessage(QByteArray(1, WS_AUTH));
}

void WebSocket::processBinaryMessage(const QByteArray& message)
{
    // qCDebug(WebSockLog) << "Binary Message received:" << message.size() << message.left(16).toHex();

    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if (!socket) return;

    auto it = client_map_.find(socket);
    if (it == client_map_.cend() || !it.value()) return;
    Websocket_Client& client = *it.value();

    QDataStream ds(message);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);

    quint8 cmd;
    uint32_t scheme_id;
    ds >> cmd >> scheme_id;

    if (scheme_id && client.last_scheme_id_queried_ != scheme_id)
        client.last_scheme_id_queried_ = scheme_id;
//    qDebug(WebSockLog) << pClient->peerAddress().toString() << pClient->peerPort() << "CMD:" << (WebSockCmd)cmd << "SCHEME" << scheme_id << "MSG" << message.size();

    if (cmd == WS_AUTH)
    {
        if (client.id_ == 0)
        {
            QByteArray token;
            ds >> token;

            if (auth(token, client))
            {
                socket->sendBinaryMessage(QByteArray(1, WS_WELCOME));
                return;
            }
            socket->close(QWebSocketProtocol::CloseCodeNormal, "Bad token");

                /*
            if (user.contains("user"))
            {
                it->isAuthenticated = true;
                it->isStaff = user["is_staff"].toBool();
                it->scheme_group_id = user["scheme_group"].toUInt();
                QJsonArray groups = user["groups"].toArray();
                for (const QJsonValue& group: groups)
                {
                    it->group_id = group.toObject()["id"].toUInt();
                    break;
                }

                pClient->sendBinaryMessage(QByteArray(1, wsWelcome));
                qCDebug(WebSockLog) << "Authenticated user:" << user["user"].toString();
                return;
            }

            qCWarning(WebSockLog) << "Bad auth token cmd:" << (int)cmd << user["error"] << django_res;
            pClient->close(QWebSocketProtocol::CloseCodeNormal, "Bad token");
            */
        }
        return;
    }
    else if (client.id_ == 0)
    {
        qCDebug(WebSockLog) << "Unathorized detected";
        socket->sendBinaryMessage(QByteArray(1, WS_AUTH));
    }
    else if (cmd < WEB_SOCK_CMD_COUNT)
    {
//        qint64 pos = ds.device()->pos();
        if (ds.device()->bytesAvailable() < 200000000) // 200mb
        {
            emit through_command(it.value(), scheme_id, cmd, ds.device()->readAll());
        }
//        ds.device()->seek(pos);
    }
    else
        std::cerr << client.id_ << ' ' << scheme_id << " Unknown WebSocket Message: " << (uint32_t)cmd << std::endl;
    /*return;

    std::shared_ptr<scheme::base> scheme = get_scheme_in_scheme_group_by_id(it->scheme_group_id, scheme_id).value_or(nullptr);

    if (cmd == ConnectInfo) {
        QByteArray msg;

        if (scheme)
        {
            QString ip;
            ip = QString::fromStdString(scheme->address());
            msg = prepare_connect_state_message(scheme_id, ip, scheme->time().zone, scheme->time().offset);
        }
        else
            msg = prepare_connect_state_message(scheme_id);

        pClient->sendBinaryMessage(msg);
        return;
    }

    if (!scheme)
        return;

    try {
        switch (cmd) {
        case WriteToDevItem: Helpz::applyParse(&scheme::base::send_value, scheme.get(), ds); break;
        case ChangeGroupMode: Helpz::applyParse(&scheme::base::send_mode, scheme.get(), ds); break;
        case ChangeParamValues: Helpz::applyParse(&scheme::base::send_param_values, scheme.get(), ds); break;
        case ChangeCode: Helpz::applyParse(&scheme::base::send_code, scheme.get(), ds); break;
        case ExecScript: Helpz::applyParse(&scheme::base::send_script, scheme.get(), ds); break;
        case Restart: scheme->send_restart(); break;

        default:
            std::cerr << scheme->title() << " Unknown WebSocket Message: " << (int)cmd << std::endl;
    //        pClient->sendBinaryMessage(message);
            break;
        }
    } catch(const std::exception& e) {
        qCCritical(WebSockLog) << "EXCEPTION: processBinaryMessage" << (int)cmd << e.what();
    }*/
}

void WebSocket::onSslErrors(const QList<QSslError> &errors)
{
    qCDebug(WebSockLog) << "SslError" << errors;
}

void WebSocket::socketDisconnected()
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if (socket)
    {
//    qCDebug(WebSockLog) << "socketDisconnected:" << pClient->peerAddress().toString() << pClient->peerPort();

        QMutexLocker lock(&clients_mutex_);
        client_map_.remove(socket);
        socket->deleteLater();
    }
}

bool WebSocket::auth(const QByteArray& token, Websocket_Client& client)
{
    try
    {
        const std::string payload = jwt_helper_->decode_and_verify(token.toStdString());

        picojson::value json_value;
        const std::string err = picojson::parse(json_value, payload);
        if (!err.empty())
            throw std::runtime_error("JSON parse error: " + err);

        if (json_value.is<picojson::object>())
        {
            const picojson::object obj = json_value.get<picojson::object>();
            const picojson::value scheme_groups_val = obj.at("groups");
            if (scheme_groups_val.is<picojson::array>())
            {
                const picojson::array scheme_groups_arr = scheme_groups_val.get<picojson::array>();
                if (!scheme_groups_arr.empty())
                {
                    for (const picojson::value& val: scheme_groups_arr)
                    {
//                        std::cout << "scheme_group_id: " << val.to_str() << std::endl;
                        client.scheme_group_id_set_.insert(val.get<int64_t>());
                    }
//                    std::cout << "user_id: " << obj.at("user_id").to_str() << std::endl;
                    client.id_ = obj.at("user_id").get<int64_t>();

//                    std::cout << "JSON PARSED" << std::endl;
                    return client.id_;
                }
            }
        }
        throw std::runtime_error("Bad payload");
    }
    catch(const std::exception& e)
    {
        qCDebug(WebSockLog) << "Token error:" << e.what();
    }
    catch(...)
    {
        qCDebug(WebSockLog) << "Token error unknown.";
    }

    return false;
}

void WebSocket::sendDevice_ItemValues(const Scheme_Info &scheme, const QVector<Log_Value_Item> &pack)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);

    ds << (quint8)WS_DEV_ITEM_VALUES << scheme.id() << (uint32_t)pack.size();
    for (const Log_Value_Item& item: pack)
        ds << item.item_id() << item.raw_value() << item.value();
    send(scheme, message);
}

void WebSocket::sendModeChanged(const Scheme_Info &scheme, const DIG_Mode &mode)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_DIG_MODE << scheme.id() << mode.mode_id() << mode.group_id();
    send(scheme, message);
}

void WebSocket::send_dig_param_values_changed(const Scheme_Info &scheme, const QVector<DIG_Param_Value> &pack)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_CHANGE_DIG_PARAM_VALUES << scheme.id() << pack;
    send(scheme, message);
}

void WebSocket::send_connection_state(const Scheme_Info &scheme, uint8_t connection_state)
{
    QByteArray msg = prepare_connection_state_message(scheme.id(), connection_state);
    send(scheme, msg);
}

void WebSocket::sendEventMessage(const Scheme_Info& scheme, const QVector<Log_Event_Item>& event_pack)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << uint8_t(WS_EVENT_LOG) << scheme.id() << event_pack;
    send(scheme, message);
}

void WebSocket::sendStatusInserted(const Scheme_Info &scheme, uint32_t group_id, uint32_t info_id, const QStringList &args)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_GROUP_STATUS_ADDED << scheme.id() << group_id << info_id << args;
    send(scheme, message);
}

void WebSocket::sendStatusRemoved(const Scheme_Info &scheme, uint32_t group_id, uint32_t info_id)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_GROUP_STATUS_REMOVED << scheme.id() << group_id << info_id;
    send(scheme, message);
}

void WebSocket::send_structure_changed(const Scheme_Info &scheme, const QByteArray &data)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_STRUCT_MODIFY << scheme.id();
    ds.writeBytes(data.constBegin(), data.size());
    send(scheme, message);
}

void WebSocket::send_time_info(const Scheme_Info& scheme, const QTimeZone& tz, qint64 time_offset)
{
    QDateTime dt = QDateTime::currentDateTime();
    dt = dt.addMSecs(time_offset);

    QString zone_name;
    if (tz.isValid())
    {
//            zone_name = tz.displayName(QTimeZone::GenericTime);
        zone_name = tz.id().constData();
        dt = dt.toTimeZone(tz);
    }

    qint64 time = dt.toMSecsSinceEpoch();

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_TIME_INFO << scheme.id() << time << zone_name;
    send(scheme, message);
}

void WebSocket::send_ip_address(const Scheme_Info& scheme, const QString& ip_address)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_IP_ADDRESS << scheme.id() << ip_address;
    send(scheme, message);
}

QByteArray WebSocket::prepare_connection_state_message(uint32_t scheme_id, uint8_t connection_state) const
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    ds << (quint8)WS_CONNECTION_STATE << scheme_id << connection_state;
    return message;
}

void WebSocket::send(const Scheme_Info &scheme, const QByteArray &data) const
{
    QMutexLocker lock(&clients_mutex_);

    qint64 curr_time = QDateTime::currentMSecsSinceEpoch();

    for (auto it = client_map_.cbegin(); it != client_map_.cend(); ++it)
    {
        if (it.value()->id_ == 0)
        {
            if ((curr_time - it.value()->auth_sended_time_) >= 500)
            {
                it.value()->auth_sended_time_ = curr_time;
                it.key()->sendBinaryMessage(QByteArray(1, WS_AUTH));
            }
        }
        else if (scheme.id() == 0 || (it.value()->last_scheme_id_queried_ == scheme.id() && scheme.check_scheme_groups(it.value()->scheme_group_id_set_)))
        {
            it.key()->sendBinaryMessage(data);
        }
    }
}

void WebSocket::send_to_client(std::shared_ptr<Websocket_Client> client, const QByteArray& data) const
{
    QMutexLocker lock(&clients_mutex_);

    for (auto it = client_map_.cbegin(); it != client_map_.cend(); ++it)
    {
        if (it.value() == client)
        {
            it.key()->sendBinaryMessage(data);
            return;
        }
    }
}

} // namespace Network
} // namespace Das
