#ifndef DAS_GUI_WEBSOCKETCLIENT_H
#define DAS_GUI_WEBSOCKETCLIENT_H

#include <QWebSocket>

#include <Das/db/dig_param_value.h>
#include <Das/param/paramgroup.h>
#include "Das/log/log_pack.h"

namespace Das {

class Proto_Scheme;

namespace Gui {

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    void set_host(const QString& host);
    void open();
signals:
    void logEventReceived(const QVector<Log_Event_Item> & event_pack);

public slots:
    void sendDevItemValue(uint32_t item_id, const QVariant& value);
    void sendGroupMode(uint32_t group_id, uint32_t mode_id);
    void send_changed_dig_param_values(const QVector<DIG_Param_Value>& pack);
    void executeCommand(const QString & text);
//    void requestLogs(int offset);
private Q_SLOTS:
    void onConnected();
    void onDisconnected();
    void onBinaryMessageReceived(const QByteArray &message);
    void onSslErrors(const QList<QSslError> &errors);
private:
    QWebSocket websock_;
    QString host_;
};

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_WEBSOCKETCLIENT_H
