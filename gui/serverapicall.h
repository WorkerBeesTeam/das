#ifndef DAS_GUI_SERVERAPICALL_H
#define DAS_GUI_SERVERAPICALL_H

#include <functional>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>

#include <QAbstractListModel>
#include <QTimer>

#include <Das/proto_scheme.h>

#include "websocketclient.h"
#include "models/journaldata.h"

#include <deque>

class QSettings;

namespace Das {
namespace Gui {

class SchemeListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    void add(uint32_t id, const QString& name, const QString& title);
    void clear();
public:
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &p = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
private:
    struct SchemeListItem {
        uint32_t id;
        QString name, title;
    };
    std::vector<SchemeListItem> items_;
};

class ServerApiCall : public QNetworkAccessManager
{
    Q_OBJECT
    Q_PROPERTY(bool authorized READ authorized NOTIFY authorizedChanged)
    Q_PROPERTY(SchemeListModel* schemeListModel READ schemeListModel NOTIFY schemeListModelChanged)

    static ServerApiCall* serverApiCall;
public:
    explicit ServerApiCall(QObject *parent = nullptr);
    ~ServerApiCall();

    static ServerApiCall* instance();

    void init();
    Proto_Scheme *prj();
    QByteArray token() const;

    bool authorized() const;
    SchemeListModel* schemeListModel();

    WebSocketClient* websock();

signals:
    void setAuthData(const QString& username, const QString& password);

    void authorizedChanged(bool authorized);
    void schemeListModelChanged();

    void detailAvailable();

    // journal
    void journalEventsInitialAvailable(const std::deque<Log_Event_Item> & events, std::size_t offset, std::size_t count);
    void journalEventsAvailable(const std::deque<Log_Event_Item> & events, std::size_t count, JournalModelData::Direction direction);
    void logEventReceived(const QVector<Log_Event_Item>& event_pack);

private slots:
    void slotError(QNetworkReply::NetworkError e);
    void slotSslErrors(const QList<QSslError> &errors);
    void refresh_token();

public slots:
    void auth(const QString& username, const QString& pwd, bool remember_me);
    void get_scheme_list();
    void get_eventlog_initial(std::size_t offset, std::size_t count);
    void get_eventlog(std::size_t id_from, std::size_t id_to, std::size_t limit, JournalModelData::Direction direction);

    void setCurrentScheme(uint32_t scheme_id, const QString& scheme_name);
    void set_changed_param_values(const QVariantList& params);
    void executeCommand(const QString& command);

private:
    void connect_reply(QNetworkReply* reply, const std::function<void()>& finished_slot);

    void authorized_call(const QString& api_url, void (ServerApiCall::*finished_slot)(QNetworkReply*));
    void authorized_call(const QString & api_url, std::function<void (QNetworkReply*)> handler);
    void auth_step2(const QString& username, const QString& pwd, bool remember_me, QNetworkReply* reply);
    void auth_step3(const QString& username, const QString& pwd, bool remember_me, QNetworkReply* reply);
    void refresh_token_complite(QNetworkReply* reply);
    void proc_scheme_list(QNetworkReply* reply);
    void proc_scheme_detail(QNetworkReply* reply);
    void proc_event_log_init(std::size_t offset, std::size_t count, QNetworkReply* reply);
    void proc_event_log(std::size_t limit, JournalModelData::Direction direction, QNetworkReply* reply);

    Log_Event_Item parseSingleJournalEvent(const QJsonValue & val);
    std::deque<Log_Event_Item> parseJournalEvents(const QJsonArray & array);

    bool m_authorized;

    QString host_;
    QByteArray csrf_token_;
    QByteArray token_;
    QVariantList permissions_;

    QSettings *s;

    SchemeListModel pl_model_;

    Proto_Scheme prj_;
    WebSocketClient websock_;

    QTimer refresh_token_timer_;
};

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_SERVERAPICALL_H
