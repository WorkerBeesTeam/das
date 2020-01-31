#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QNetworkRequest>
#include <QNetworkCookieJar>
#include <QNetworkCookie>

#include <QSettings>
#include <QDebug>

#ifdef DAS_WITH_LOGGING
#include <Helpz/settingshelper.h>
#include <Helpz/logging.h>
#endif

#include <functional>
#include <deque>

#include <Das/device.h>
#include <Das/db/device_item_value.h>

#include "serverapicall.h"

namespace Das {
namespace Gui {

const char id_str[] = "id";
const char name_str[] = "name";
const char title_str[] = "title";

using namespace std::placeholders;

void SchemeListModel::add(uint32_t id, const QString &name, const QString &title) {
    int row = items_.size();
    beginInsertRows(QModelIndex(), row, row);
    items_.push_back({id, name, title});
    endInsertRows();
}

void SchemeListModel::clear() {
    beginResetModel();
    items_.clear();
    endResetModel();
}

QHash<int, QByteArray> SchemeListModel::roleNames() const {
    return {
        { Qt::DisplayRole, title_str },
        { Qt::EditRole,    name_str },
        { Qt::UserRole,    id_str }
    };
}

int SchemeListModel::rowCount(const QModelIndex &/*p*/) const { return items_.size(); }

QVariant SchemeListModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= static_cast<int>(items_.size()) || idx.row() < 0)
        return {};

    const SchemeListItem& item = items_.at(idx.row());

    switch (role) {
    case Qt::DisplayRole: return item.title;
    case Qt::EditRole:    return item.name;
    case Qt::UserRole:    return item.id;
    default:
        break;
    }
    return {};
}

#ifdef DAS_WITH_LOGGING
typedef Helpz::SettingsThreadHelper<Helpz::Logging, bool
#ifdef Q_OS_UNIX
    , bool
#endif
        > LogThread;
LogThread::Type* g_logThread;
#endif

ServerApiCall::ServerApiCall(QObject *parent) :
    QNetworkAccessManager(parent),
    m_authorized(false)
{
//    DBus::registerMetaType();

//    prj_ = new ClientScheme(this);
//    m_ctrl = new ru::deviceaccess::Das::Client("ru.deviceaccess.Das.Client", "/",
//                                                QDBusConnection::systemBus(), this);

//    connect(ctrl(), SIGNAL(change(const Das::PackItem&, bool)),
//            prj(), SLOT(setChange(const Das::PackItem&)));

//    connect(ctrl(), SIGNAL(modeChanged(uint,uint,uint)),
//            prj(), SLOT(setMode(uint,uint,uint)));

//    connect(ctrl(), &Control::groupStatusChanged, prj(), &Proto_Scheme::setStatus);

////    connect(prj(), SIGNAL(toggle(uint,bool)), dbus(), SLOT(toggle(uint,bool)));

    if (qEnvironmentVariableIsSet("DAS_GUI_CONF"))
        s = new QSettings(qgetenv("DAS_GUI_CONF"), QSettings::NativeFormat, this);
    else
        s = new QSettings(this);

#ifdef DAS_WITH_LOGGING
    g_logThread = LogThread()(
              s, "Log", Helpz::Param<bool>{"Debug", false}
#ifdef Q_OS_UNIX
                ,Helpz::Param<bool>{"Syslog", true}
#endif
            );
    g_logThread->start();
#endif

    s->beginGroup("ApiServer");

    serverApiCall = this;

    host_ = s->value("Host", "deviceaccess.ru").toString();
    if (!s->contains("Host"))
        s->setValue("Host", host_);
    qDebug() << "Use server:" << host_;
    refresh_token_timer_.setSingleShot(false);
    refresh_token_timer_.setInterval(3 * 60 * 1000);
    connect(&refresh_token_timer_, &QTimer::timeout, this, &ServerApiCall::refresh_token);
    websock_.set_host("wss://" + host_ + "/wss/");
    host_ = "https://" + host_;

    refresh_token_timer_.setSingleShot(false);
    refresh_token_timer_.setInterval(3 * 60 * 1000);
    connect(&refresh_token_timer_, &QTimer::timeout, this, &ServerApiCall::refresh_token);

    connect(&websock_, &WebSocketClient::logEventReceived, this, &ServerApiCall::logEventReceived);

//    connect(this, &QNetworkAccessManager::finished, this, &ServerApiCall::replyFinished);
    //    setCookieJar(new QNetworkCookieJar(this));
}

ServerApiCall::~ServerApiCall()
{
#ifdef DAS_WITH_LOGGING
    g_logThread->quit(); g_logThread->wait();
    delete g_logThread;
#endif
}

/*static*/ServerApiCall* ServerApiCall::serverApiCall = nullptr;
/*static*/ServerApiCall* ServerApiCall::instance() { return serverApiCall; }

void ServerApiCall::init() {
    emit setAuthData(s->value("Username").toString(), s->value("Password").toString());
}

Proto_Scheme *ServerApiCall::prj() { return &prj_; }
QByteArray ServerApiCall::token() const { return token_; }

bool ServerApiCall::authorized() const { return m_authorized; }
SchemeListModel *ServerApiCall::schemeListModel() { return &pl_model_; }

WebSocketClient *ServerApiCall::websock() { return &websock_; }

void ServerApiCall::slotError(QNetworkReply::NetworkError e) {
    qCritical() << "slotError" << e;
}

void ServerApiCall::slotSslErrors(const QList<QSslError> &errors)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
#ifndef QT_DEBUG
    if (!host_.toLower().startsWith("https://deviceaccess.ru"))
#endif
    {
        reply->ignoreSslErrors();
        qWarning() << "SSL Errors ignore";
    }
    qCritical() << "HTTP SSL errors:";
    for (const QSslError& err: errors)
        qCritical() << '-' << (int)err.error() << err.errorString();
}

void ServerApiCall::refresh_token()
{
    if (csrf_token_.isEmpty()) {
        qWarning() << "CSRF Token empty";
        return;
    }

    QJsonObject json;
    json["token"] = token_.constData();
    QByteArray param = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req(host_ + "/api/token/refresh/");
    req.setRawHeader("X-CSRFToken", csrf_token_);
    req.setRawHeader("Authorization", "JWT " + token_);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//    req.setHeader(QNetworkRequest::ContentLengthHeader, param.size());
    QNetworkReply* reply = post(req, param);
    connect_reply(reply, std::bind(&ServerApiCall::refresh_token_complite, this, reply));
}

void ServerApiCall::auth(const QString &username, const QString &pwd, bool remember_me)
{
    refresh_token_timer_.stop();
    QNetworkReply* reply = head(QNetworkRequest(QUrl(host_)));
    connect_reply(reply, std::bind(&ServerApiCall::auth_step2, this, username, pwd, remember_me, reply));
}


void ServerApiCall::get_scheme_list() {
    authorized_call("scheme/?limit=100&offset=0&ordering=title", &ServerApiCall::proc_scheme_list);
}

void ServerApiCall::get_eventlog_initial(std::size_t offset, std::size_t count) {
    auto query = QString("events/?limit=%3&offset=%2&ordering=-id&id=%1")
            .arg(prj_.id())
            .arg(offset)
            .arg(count);

    std::function<void (QNetworkReply*)> handler =
            std::bind(&ServerApiCall::proc_event_log_init, this, offset, count, _1);

    authorized_call(query, handler);
}

void ServerApiCall::get_eventlog(std::size_t id_from, std::size_t id_to, std::size_t limit, JournalModelData::Direction direction)
{
    auto query =
            QString("events/?limit=%3&pk_from=%1&pk_to=%2&ordering=-id&id=1")
                .arg(id_from)
                .arg(id_to)
                .arg(limit);

    auto handler = std::bind(&ServerApiCall::proc_event_log, this, limit, direction, _1);

    authorized_call(query, handler);
}

void ServerApiCall::setCurrentScheme(uint32_t scheme_id, const QString& scheme_name) {
    prj_.set_id(scheme_id);
    authorized_call("detail/?scheme_name=" + scheme_name, &ServerApiCall::proc_scheme_detail);
}


void ServerApiCall::set_changed_param_values(const QVariantList &params)
{
    QVector<DIG_Param_Value> pack;
    QVariantList param_pair;
    for (const QVariant& p: params)
    {
        param_pair = p.toList();
        qDebug() << "adding param, " << param_pair;
        if (param_pair.length() == 2)
            pack.push_back({param_pair.at(0).toUInt(), param_pair.at(1).toString()});
    }
    qDebug() << "Write params" << params;
    qDebug() << "Count = " << pack.count();
    websock_.send_changed_dig_param_values(pack);
}


void ServerApiCall::executeCommand(const QString &command)
{
    websock_.executeCommand(command);
}


void ServerApiCall::connect_reply(QNetworkReply *reply, const std::function<void ()>& finished_slot)
{
    connect(reply, &QNetworkReply::finished, finished_slot);
    connect(reply, &QNetworkReply::sslErrors, this, &ServerApiCall::slotSslErrors);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &ServerApiCall::slotError);
}


void ServerApiCall::authorized_call(const QString &api_url, void (ServerApiCall::*finished_slot)(QNetworkReply*))
{
    QNetworkRequest req(host_ + "/api/v1/" + api_url);
    req.setRawHeader("Authorization", "JWT " + token_);

    QNetworkReply* reply = get(req);
    connect_reply(reply, std::bind(finished_slot, this, reply));
}


void ServerApiCall::authorized_call(const QString & api_url, std::function<void (QNetworkReply *)> handler)
{
    QNetworkRequest req(host_ + "/api/v1/" + api_url);
    req.setRawHeader("Authorization", "JWT " + token_);

    QNetworkReply* reply = get(req);
    connect_reply(reply, std::bind(handler, reply));
}


void ServerApiCall::auth_step2(const QString &username, const QString &pwd, bool remember_me, QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    auto cookie_list = cookieJar()->cookiesForUrl(reply->url());
    reply->deleteLater();

    csrf_token_.clear();
    for (const QNetworkCookie& cookie: cookie_list)
        if (cookie.name().toLower() == "csrftoken") {
            csrf_token_ = cookie.value();
            break;
        }

    if (csrf_token_.isEmpty())
        return;

    QJsonObject json;
    json["username"] = username;
    json["password"] = pwd;
    QByteArray param = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req(host_ + "/api/token/auth/");
    req.setRawHeader("X-CSRFToken", csrf_token_);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//    req.setHeader(QNetworkRequest::ContentLengthHeader, param.size());
    reply = post(req, param);
    connect_reply(reply, std::bind(&ServerApiCall::auth_step3, this, username, pwd, remember_me, reply));
}


void ServerApiCall::auth_step3(const QString& username, const QString& pwd, bool remember_me, QNetworkReply *reply)
{
    refresh_token_complite(reply);
    refresh_token_timer_.start();

    s->setValue("Username", username);
    if (remember_me)
        s->setValue("Password", pwd);
    s->endGroup();

    qDebug() << "Auth finish";// << reply->rawHeaderPairs() << reply->errorString();

    emit authorizedChanged(m_authorized = true);
    get_scheme_list();
}

void ServerApiCall::refresh_token_complite(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError || data.isEmpty())
        return;

    QJsonObject json = QJsonDocument::fromJson(data).object();
    if (!json.contains("token"))
        return;

    token_ = json.value("token").toString().toLocal8Bit();
    permissions_ = json.value("permissions").toArray().toVariantList();
}


void ServerApiCall::proc_scheme_list(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError || data.isEmpty())
        return;

    QJsonArray json_array = QJsonDocument::fromJson(data).object().value("results").toArray();
    QJsonObject json;

    pl_model_.clear();
    for (const QJsonValue& json_val: json_array) {
        json = json_val.toObject();
        pl_model_.add(json.value(id_str).toInt(),
                      json.value(name_str).toString(),
                      json.value(title_str).toString());
    }

    qDebug() << "SchemeList received size:" << pl_model_.rowCount();
}


void ServerApiCall::proc_scheme_detail(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError || data.isEmpty())
        return;

    qDebug() << "Scheme detail received";
    QJsonObject doc = QJsonDocument::fromJson(data).object(), json;

    prj_.device_item_type_mng_.clear();
    QJsonArray json_arr = doc.value("itemTypes").toArray();
    for (const QJsonValue& json_val: json_arr) {
        json = json_val.toObject();
        prj_.device_item_type_mng_.add({ static_cast<uint32_t>(json.value(id_str).toInt()),
                               json.value(name_str).toString(),
                               json.value(title_str).toString(),
                               static_cast<uint32_t>(json.value("group_type_id").toInt()),
                               static_cast<uint32_t>(json.value("sign_id").toInt()),
                               static_cast<quint8>(json.value("register_type").toInt()),
                               static_cast<quint8>((json.value("save_immediately").toBool() ? Device_Item_Type::SA_IMMEDIATELY : Device_Item_Type::SA_BY_TIMER)),
                             });
    }

    prj_.group_type_mng_.clear();
    json_arr = doc.value("groupTypes").toArray();
    for (const QJsonValue& json_val: json_arr) {
        json = json_val.toObject();
        prj_.group_type_mng_.add({ static_cast<uint>(json.value(id_str).toInt()),
                                json.value(name_str).toString(),
                                json.value(title_str).toString(),
                                json.value("description").toString()
                              });
    }

    prj_.sign_mng_.clear();
    json_arr = doc.value("signTypes").toArray();
    for (const QJsonValue& json_val: json_arr) {
        json = json_val.toObject();
        prj_.sign_mng_.add({ static_cast<uint32_t>(json.value(id_str).toInt()),
                           json.value(name_str).toString() });
    }

    prj_.param_mng_.clear();
    json_arr = doc.value("params").toArray();
    for (const QJsonValue& json_val: json_arr) {
        json = json_val.toObject();
        prj_.param_mng_.add({ static_cast<uint>(json.value(id_str).toInt()),
                            json.value(name_str).toString(),
                            json.value(title_str).toString(),
                            json.value("description").toString(),
                            (DIG_Param_Type::Value_Type)json.value("type").toInt(),
                            static_cast<uint>(json.value("groupType_id").toInt()),
                            static_cast<uint>(json.value("parent_id").toInt()) });
    }

    prj_.clear_sections();
    json_arr = doc.value("sections").toArray();
    QJsonArray groups_arr, params_arr;
    Section* sct;
    Device_item_Group* group;
    QVector<Device_item_Group *> groups;
    QVector<std::shared_ptr<Param>> lostParams;
    std::shared_ptr<Param> param;
    for (const QJsonValue& json_val: json_arr)
    {
        json = json_val.toObject();
        Section section(json.value(id_str).toInt(), json.value(name_str).toString(),
                               static_cast<uint32_t>(json.value("dayStart").toInt()),
                               static_cast<uint32_t>(json.value("dayEnd").toInt()) );

        sct = prj_.add_section(std::move(section));
        groups_arr = json.value("groups").toArray();
        for (const QJsonValue& group_val: groups_arr)
        {
            json = group_val.toObject();
            Database::Device_Item_Group item_group( json.value(id_str).toInt(), json.value("title").toString(), sct->id(), json.value("type_id").toInt() );

            group = sct->add_group(std::move(item_group), json.value("mode_id").toInt());
            groups.push_back(group);
            params_arr = json.value("params").toArray();
            for (const QJsonValue& param_val: params_arr) {
                json = param_val.toObject();
                param = std::make_shared<Param>( json.value(id_str).toInt(),
                                                 prj_.param_mng_.get_type(json.value("param_id").toInt()),
                                                 json.value("value").toString() );
                if (!group->params()->add(param))
                    lostParams.push_back(param);
            }

            for (std::shared_ptr<Param>& param: lostParams)
                group->params()->add(param);
            lostParams.clear();
        }
    }

    prj_.clear_devices();
    json_arr = doc.value("devices").toArray();
    Device* dev;
    Device_Item* dev_item;
    uint32_t parent_id;
    std::map<uint32_t, std::vector<Device_Item*>> items_for_group;
    std::vector<Device_Item*> dev_items;
    QVector<QPair<Device_Item*, uint32_t>> itemTree;
    QJsonArray item_arr;
    for (const QJsonValue& json_val: json_arr)
    {
        json = json_val.toObject();

        Device device(json.value(id_str).toInt(),
                      json.value(name_str).toString(),
                      json.value("extra").toObject().toVariantMap(),
                      json.value("checker_id").toInt());
        dev = prj_.add_device( std::move(device) );
        item_arr = json.value("items").toArray();
        for (const QJsonValue& item_val: item_arr)
        {
            json = item_val.toObject();
            Device_Item device_item( json.value(id_str).toInt(),
                                    json.value(name_str).toString(),
                                    json.value("type_id").toInt(),
                                    QJsonDocument::fromJson(json.value("extra").toString().toUtf8()).array().toVariantList(),
                                    json.value("parent_id").toInt(), json.value("device_id").toInt(), json.value("group_id").toInt());
            QJsonObject val_obj = json.value("val").toObject();
            if (!val_obj.isEmpty())
                device_item.set_data(Device_Item_Value::variant_from_string(val_obj.value("raw").toVariant()),
                                    Device_Item_Value::variant_from_string(val_obj.value("display").toVariant()));
            dev_item = dev->create_item(std::move(device_item));
            dev_items.push_back(dev_item);
            items_for_group[json.value("group_id").toInt()].push_back(dev_item);
            parent_id = json.value("parent_id").toInt();
            if (parent_id)
                itemTree.push_back(QPair<Device_Item*, uint32_t>(dev_item, parent_id));
        }
    }

    for (const QPair<Device_Item*, uint32_t>& child: itemTree) {
        for (Device_Item* item: dev_items)
            if (item != child.first && item->id() == child.second) {
                child.first->set_parent(item);
                break;
            }
    }
    for (const std::pair<uint32_t, std::vector<Device_Item*>>& item: items_for_group) {
        for (Device_item_Group* group_it: groups) {
            if (group_it->id() == item.first) {
                for (Device_Item* dev_it: item.second)
                    dev_it->set_group(group_it);
                break;
            }
            group_it->finalize();
        }
    }

    websock_.open();
    emit detailAvailable();
}

void ServerApiCall::proc_event_log_init(std::size_t offset, std::size_t count, QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError || data.isEmpty())
       return;

    QJsonArray jsonArray = QJsonDocument::fromJson(data).object().value("results").toArray();
//    qDebug() << jsonArray;
    std::deque<Log_Event_Item> events = parseJournalEvents(jsonArray);
    emit journalEventsInitialAvailable(events, offset, count);
}


void ServerApiCall::proc_event_log(std::size_t limit, JournalModelData::Direction direction, QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError || data.isEmpty())
       return;

    QJsonArray jsonArray = QJsonDocument::fromJson(data).object().value("results").toArray();
//    qDebug() << jsonArray;
    std::deque<Log_Event_Item> events = parseJournalEvents(jsonArray);
    emit journalEventsAvailable(events, limit, direction);
}


Log_Event_Item ServerApiCall::parseSingleJournalEvent(const QJsonValue &val)
{
    auto json = val.toObject();
    QDateTime date = QDateTime::fromString(json["date"].toString(), Qt::ISODateWithMs);

    return {
        date.toMSecsSinceEpoch(), static_cast<uint32_t>(json["user_id"].toInt()), false,
        static_cast<uint8_t>(json["type"].toInt()), json["who"].toString(), json["msg"].toString()
    };
}


std::deque<Log_Event_Item> ServerApiCall::parseJournalEvents(const QJsonArray &array)
{
    std::deque<Log_Event_Item> result;

    QJsonObject json;
    for (const QJsonValue& json_val: array) {
        result.push_back(parseSingleJournalEvent(json_val));
    }

    return result;
}

} // namespace Gui
} // namespace Das
