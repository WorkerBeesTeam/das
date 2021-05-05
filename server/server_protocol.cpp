#include <QJsonDocument>
#include <QCryptographicHash>
#include <QFile>

#include <Helpz/dtls_server.h>
#include <Helpz/dtls_server_node.h>
#include <Helpz/dtls_server_thread.h>

#include <Das/commands.h>

#include "dbus_object.h"
#include "server.h"
#include "server_protocol.h"

namespace Das {
namespace Ver {
namespace Server {

Protocol::Protocol(Worker *work_object) :
    Protocol_Base{ work_object },
    is_copy_(false),
    disable_sync_(false),
    structure_sync_(this)
{
}

Protocol::~Protocol()
{
    if (/*!is_copy_ &&*/ id())
    {
        std::cout << "~Protocol " << id() << " is copy: " << std::boolalpha << is_copy_ << std::endl;
//        std::cout << "closed " << id() << " is copy: " << is_copy_ << std::endl;
        work_object()->recently_connected_.disconnected(*this);
        set_connection_state(CS_DISCONNECTED_JUST_NOW);
    }
}

void Protocol::disable_sync()
{
    disable_sync_ = true;
}

Structure_Synchronizer* Protocol::structure_sync()
{
    return &structure_sync_;
}

int Protocol::protocol_version() const
{
    return 206;
}

void Protocol::send_file(uint32_t user_id, uint32_t dev_item_id, const QString& file_name, const QString& file_path)
{
    std::unique_ptr<QFile> device(new QFile(file_path));
    if (!device->open(QIODevice::ReadOnly))
    {
        qWarning().noquote() << title() << "Can't open file:" << device->errorString();
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(device.get()))
    {
        qWarning().noquote() << title() << "Can't get file hash";
        return;
    }

    QByteArray item_value_data;
    QDataStream ds(&item_value_data, QIODevice::WriteOnly);
    ds.setVersion(DATASTREAM_VERSION);
    ds << file_name << hash.result();

    qDebug().noquote() << title() << "send file" << file_name << "for devitem" << dev_item_id
                       << "file_path" << file_path << "hash sha1" << hash.result().toHex().constData();

    QVariant raw_data = QVariant::fromValue(item_value_data);
    send(Cmd::WRITE_TO_ITEM) << user_id << dev_item_id << raw_data;

    device->seek(0);
    send(Cmd::WRITE_TO_ITEM_FILE).set_data_device(std::move(device));
}

void Protocol::send_start_stream(uint32_t user_id, uint32_t dev_item_id, const QString &url)
{
    send(Cmd::STREAM_START).answer([this, user_id, dev_item_id](QIODevice& data_dev)
    {
        apply_parse(data_dev, &Protocol::stream_started, user_id, dev_item_id);
    }).timeout(nullptr, std::chrono::seconds(8)) << user_id << dev_item_id << url;
}

void Protocol::synchronize(bool full)
{
    if (!disable_sync_)
        structure_sync_.check(full ? structure_sync_.modified() : true, true);
}

void Protocol::set_scheme_name(uint32_t user_id, const QString &name)
{
    send(Cmd::SET_SCHEME_NAME).timeout(nullptr, std::chrono::seconds(8)) << user_id << name;
}

void Protocol::closed()
{
}

void Protocol::before_remove_copy()
{
    is_copy_ = true;
}

void Protocol::lost_msg_detected(uint8_t /*msg_id*/, uint8_t /*expected*/)
{
    set_connection_state(connection_state() | CS_CONNECTED_WITH_LOSSES);

    const auto now = std::chrono::system_clock::now();
    if (!disable_sync_ && now - last_sync_time_ > std::chrono::seconds(7))
    {
        last_sync_time_ = now;
        log_sync_check();
    }
}

void Protocol::ready_write()
{
    qDebug().noquote() << title() << "CONNECTED";
}

void Protocol::process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    if (!id())
    {
        process_unauthorized_message(msg_id, cmd, data_dev);
        return;
    }

    switch (cmd)
    {

    case Cmd::LOG_DATA_REQUEST:
    case Cmd::LOG_PACK:
        if (Helpz::apply_parse(data_dev, DATASTREAM_VERSION,
                               &Log_Synchronizer::process, &_log_sync, &data_dev, *this, cmd == Cmd::LOG_DATA_REQUEST))
        {
            send_answer(cmd, msg_id);
            if (cmd == Cmd::LOG_DATA_REQUEST)
                log_sync_check();
        }
        break;

    case Cmd::MODIFY_SCHEME:
        send_answer(cmd, msg_id);

        Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Structure_Synchronizer::process_modify, &structure_sync_, &data_dev);
        if (structure_sync_.modified() != bool(connection_state() & CS_CONNECTED_MODIFIED))
        {
            set_connection_state((connection_state() & ~CS_CONNECTED_MODIFIED) | (structure_sync_.modified() ? CS_CONNECTED_MODIFIED : 0));
        }
        break;

    default:
        break;
    }
}

void Protocol::process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice& /*data_dev*/)
{
    qDebug().noquote() << title() << "unprocess answer" << msg_id << int(cmd) << static_cast<Cmd::Command_Type>(cmd) << QDateTime::currentMSecsSinceEpoch();
}

void Protocol::process_unauthorized_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    switch (cmd)
    {

    case Cmd::AUTH:
        apply_parse(data_dev, &Protocol::auth, msg_id);
        return;

    default:
        send(Cmd::NO_AUTH);
        qDebug().noquote() << title() << "Havnt auth. CMD:" << int(cmd);
        // TODO: Послать сигнал о неудаче что бы сервер удалил этого клиента?
        break;
    }
}

void Protocol::auth(const Authentication_Info &info, bool modified, uint8_t msg_id)
{
    QUuid device_connection_id;
    if (info)
    {
        db()->check_auth(info, this, device_connection_id);
    }

    bool authenticated = id();

    qDebug() << qPrintable(title()) << "authentication" << info.scheme_name() << "login" << info.login()
              << info.using_key() << "status:" << (authenticated ? "true" : "false");
    send_answer(Cmd::AUTH, msg_id) << authenticated << device_connection_id;

    if (authenticated)
    {
        work_object()->server_thread_->server()->remove_copy(this);

        work_object()->save_connection_state_to_log(id(), std::chrono::system_clock::now(), /*state=*/true);

        structure_sync_.set_modified(modified);
        set_connection_state(CS_CONNECTED_JUST_NOW);
        std::cout << "Connected state " << id() << std::endl;

        if (modified)
            set_connection_state(connection_state() | CS_CONNECTED_MODIFIED);

        send(Cmd::VERSION).answer([this](QIODevice& data_dev) { print_version(data_dev); });
        send(Cmd::TIME_INFO).answer([this](QIODevice& data_dev) { apply_parse(data_dev, &Protocol::set_time_offset); });

        if (!disable_sync_)
        {
            structure_sync_.check(modified);
            log_sync_check();
        }

        if (!work_object()->recently_connected_.remove(id()))
        {
            // db()->deferred_clear_status(name());
        }
    }
    else
    {
        // TODO: послать сигнал о неудаче что бы сервер удалил этого клиента
    }
}

QString Protocol::concat_version(quint8 v_major, quint8 v_minor, uint32_t v_build)
{
    return QString("%1.%2.%3").arg(v_major).arg(v_minor).arg(v_build);
}

void Protocol::print_version(QIODevice& data_dev)
{
    QJsonObject obj_helpz;
    obj_helpz["DTLS"] = apply_parse(data_dev, &Protocol::concat_version);
    obj_helpz["Network"] = apply_parse(data_dev, &Protocol::concat_version);
    obj_helpz["Database"] = apply_parse(data_dev, &Protocol::concat_version);
    obj_helpz["Service"] = apply_parse(data_dev, &Protocol::concat_version);

    QJsonObject obj;
    obj["Helpz"] = obj_helpz;
    obj["DasLibrary"] = apply_parse(data_dev, &Protocol::concat_version);
    obj["Client"] = apply_parse(data_dev, &Protocol::concat_version);

    set_version(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    qDebug().noquote() << title() << version();
}

void Protocol::set_time_offset(const QDateTime& scheme_time, const QTimeZone& timeZone)
{
    auto curr_dt = QDateTime::currentDateTimeUtc();
    qint64 offset = scheme_time.toUTC().toMSecsSinceEpoch() - curr_dt.toMSecsSinceEpoch();
    set_time_param(timeZone, offset);

    QMetaObject::invokeMethod(work_object()->dbus_, "time_info", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *this), Q_ARG(QTimeZone, timeZone), Q_ARG(qint64, offset));

    qDebug().noquote() << title() << "setTimeOffset" << time().offset
                       << (time().zone.isValid() ? time().zone.id().constData()/*displayName(QTimeZone::GenericTime).toStdString()*/ : "invalid");
}

void Protocol::stream_started(const QByteArray &data, uint32_t user_id, uint32_t dev_item_id)
{
    QMetaObject::invokeMethod(work_object()->dbus_, "stream_started", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *this), Q_ARG(uint32_t, user_id),
                              Q_ARG(uint32_t, dev_item_id), Q_ARG(QByteArray, data));
}

void Protocol::log_sync_check()
{
    std::set<uint8_t> log_types = _log_sync.get_type_set();

    qCDebug(Sync_Log).noquote() << title() << "request_log_data";
    for (uint8_t log_type: log_types)
    {
        send(Cmd::LOG_DATA_REQUEST).timeout([this, log_type]()
        {
            qCWarning(Sync_Log).noquote() << title() << int(log_type) << "request_log_range timeout";
        }, std::chrono::seconds(15)) << log_type;
    }
}

#if 0
#include <iostream>
#include <memory>

#include <QDebug>
#include <QElapsedTimer>

#include <Das/device_item.h>
#include <Das/param/paramgroup.h>
#include <Das/typemanager/typemanager.h>

#include "database/db_scheme.h"

class protocol_impl
{
public:
    void closed()
    {
        //    webSock_th->ptr()->sendConnectState(this, false);
        // Maybe write disconnect time
    }
    void process_message_stream(uint16_t cmd, QDataStream &msg)
    {
        switch (cmd) {
        case cmdGetServerInfo:
        {
            struct ServerNodeData {
                std::shared_ptr<scheme_manager> manager;
                QVector<QPair<QUuid, QString>> devices;
            };
            //            ServerNodeData client = apply_parse(&server_node_auth_protocol::auth, msg);
            //            if (client.manager)
            //            {
            //                auto&& helper = send(cmdServerInfo);
            //                client.manager->dumpInfoToStream(&helper.dataStream());
            //            }
            return;
        }
        case cmdCreateDevice:
        {
            //            uint32_t scheme_group_id = apply_parse(&server_node_auth_protocol::getUserscheme_groupId, msg);
            //            if (scheme_group_id)
            //            {
            //                QString name, latin, description;
            //                msg >> name >> latin >> description;
            //                if (!name.isEmpty() && !latin.isEmpty())
            //                {
            //                    QUuid device_uuid = createDevice(scheme_group_id, name, latin, description);
            //                    if (!device_uuid.isNull())
            //                        send(cmdCreateDevice) << device_uuid;
            //                }
            //            }
            return;
        }
        default: break;
        }
    }
private:
    void send_schemes(const QString &login, const QString &password)
    {
        QVector<QPair<QUuid, QString>> scheme_list = db()->getUserDevices(login, password);
        send(cmdSchemes) << scheme_list;
    }

    void setStatus(uint32_t group_id, uint32_t status)
    {
        //    std::cout << title() << " setStatus " << group_id << ' ' << status << std::endl;
        /*
    auto group = Proto_Scheme::setStatus(group_id, status);
    if (group)
    {
        GroupStatusInfo info = group->getStatusInfo();

        QMutexLocker lock(&msgCacheMutex);
        if (info.inform())
        {
            msgCache[group->section()->id()].insert(group_id);
            if (!m_msgTimer.isActive())
                m_msgTimer.start();
        }
        else
        {
            // Remove from cache if still not sended
            auto sct_it = msgCache.find(group->section()->id());
            if (sct_it != msgCache.end())
                sct_it->second.erase(group_id);

            // Stop timer if cache empty
            if (!msgCache.empty())
            {
                bool empty = true;
                for(const std::pair<uint32_t, std::set<uint32_t>>& it: msgCache)
                    if (!it.second.empty()) {
                        empty = false;
                        break;
                    }
                if (empty) {
                    m_msgTimer.stop();
                    msgCache.clear();
                }
            }

            // Inform about resume
            if (informCache.erase(group_id) > 0)
                emit criticalMessage( QString("%1:\n%2 %3")
                                      .arg(name())
                                      .arg(group->section()->toString())
                                      .arg(groupStatusStringConcat(group, "Всё хорошо!")) );
        }
    }*/
    }

    std::shared_ptr<scheme_manager> mng_;
};
#endif

} // namespace Server
} // namespace Ver
} // namespace Das
