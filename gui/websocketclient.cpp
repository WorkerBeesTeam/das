#include <QDebug>
#include <QDataStream>

#include <Helpz/apply_parse.h>
#include <Das/proto_scheme.h>
#include <Das/commands.h>

#include "serverapicall.h"
#include "websocketclient.h"

namespace Das {
namespace Gui {

WebSocketClient::WebSocketClient(QObject *parent) :
    QObject(parent)
{
    connect(&websock_, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&websock_, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(&websock_, &QWebSocket::binaryMessageReceived,
            this, &WebSocketClient::onBinaryMessageReceived);
    connect(&websock_, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors),
            this, &WebSocketClient::onSslErrors);
}

void WebSocketClient::set_host(const QString &host) { host_ = host; }
void WebSocketClient::open() {
    qDebug() << "WebSock open" << host_;
    websock_.open(QUrl(host_));
}

void WebSocketClient::sendDevItemValue(uint32_t item_id, const QVariant &value)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << static_cast<quint8>(WS_WRITE_TO_DEV_ITEM)
       << ServerApiCall::instance()->prj()->id()
       << item_id << value;

    websock_.sendBinaryMessage(data);
}

void WebSocketClient::sendGroupMode(uint32_t group_id, uint32_t mode_id)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << static_cast<quint8>(WS_CHANGE_DIG_MODE_ITEM)
       << ServerApiCall::instance()->prj()->id()
       << mode_id << group_id;

    websock_.sendBinaryMessage(data);
}

void WebSocketClient::send_changed_dig_param_values(const QVector<DIG_Param_Value> &pack)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << static_cast<quint8>(WS_CHANGE_DIG_PARAM_VALUES)
       << ServerApiCall::instance()->prj()->id()
       << pack;

    websock_.sendBinaryMessage(data);
}

void WebSocketClient::executeCommand(const QString &text)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << static_cast<quint8>(WS_EXEC_SCRIPT)
       << ServerApiCall::instance()->prj()->id()
       << text;

    websock_.sendBinaryMessage(data);
}

void WebSocketClient::onConnected()
{
    qDebug() << "WebSocket connected";
    //    websock_.sendTextMessage(QStringLiteral("Hello, world!"));
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "WebSocket disconnected";
}

void WebSocketClient::onBinaryMessageReceived(const QByteArray &message)
{
    Proto_Scheme* prj = ServerApiCall::instance()->prj();

    quint8 cmd;
    QDataStream msg(message);
    msg >> cmd;
    try {
        if (cmd == WS_AUTH || cmd == WS_WELCOME) {
            QByteArray data;
            QDataStream ds(&data, QIODevice::WriteOnly);
            if (cmd == WS_AUTH)
                ds << cmd << prj->id() << ServerApiCall::instance()->token();
            else {
                qDebug() << "WebSock auth successful";
                ds << (quint8)WS_CONNECTION_STATE << prj->id();
            }
            websock_.sendBinaryMessage(data);
        } else {
            uint32_t scheme_id;
            msg >> scheme_id;
            if (prj->id() != scheme_id)
                return;

            switch (cmd) {
            case WS_CONNECTION_STATE: {
                bool connected;
                qint64 time;
                QString ip, zone_name;
                Helpz::parse_out(msg, connected, ip, time, zone_name);
                qDebug() << scheme_id << "connected" << connected << ip << time << zone_name;
                // Todo: ...
                break;
            }
            case WS_DEV_ITEM_VALUES: {
                uint32_t size;
                Helpz::parse_out(msg, size);
                while (size--)
                    Helpz::apply_parse(msg, &Proto_Scheme::setChangeImpl, prj);
                break;
            }
            case WS_EVENT_LOG:
                Helpz::apply_parse(msg, &WebSocketClient::logEventReceived, this);
                break;
            case WS_DIG_MODE_ITEM:
                Helpz::apply_parse(msg, &Proto_Scheme::set_mode, prj);
                break;
            case WS_CHANGE_DIG_PARAM_VALUES:
                Helpz::apply_parse(msg, &Proto_Scheme::set_dig_param_values, prj);
                break;
            default:
                break;
            }
        }
    } catch(const std::exception& e) {
        qCritical() << "WebSock exception" << e.what() << "CMD" << cmd << message.toHex();
    } catch(...) {
        qCritical() << "WebSock Unknown exception";
    }
}

void WebSocketClient::onSslErrors(const QList<QSslError> &errors)
{
#ifndef QT_DEBUG
    if (!host_.toLower().startsWith("wss://deviceaccess.ru/wss"))
#endif
        websock_.ignoreSslErrors();
    qCritical() << "WebSocket SSL errors:";
    for (const QSslError& err: errors)
        qCritical() << '-' << err.error() << err.errorString();
}

} // namespace Gui
} // namespace Das
