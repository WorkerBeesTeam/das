#include <QJsonDocument>
#include <QCryptographicHash>
#include <QFile>

#include <Helpz/dtls_server.h>
#include <Helpz/dtls_server_node.h>
#include <Helpz/dtls_server_thread.h>

#include <Das/commands.h>

#include "dbus_object.h"
#include "server.h"
#include "server_protocol_2_1.h"

namespace Das {
namespace Ver_2_1 {
namespace Server {

Protocol::Protocol(Work_Object* work_object) :
    Protocol_Base{ work_object },
    is_copy_(false),
    log_sync_(this),
    structure_sync_(this)
{
    structure_sync_.set_common_db_name(db_conn_info->db_name());
}

Protocol::~Protocol()
{
    if (!is_copy_ && id())
    {
        work_object()->recently_connected_.disconnected(*this);
        set_connection_state(CS_DISCONNECTED_JUST_NOW);
    }
}

Structure_Synchronizer* Protocol::structure_sync()
{
    return &structure_sync_;
}

int Protocol::protocol_version() const
{
    return 201;
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

void Protocol::synchronize(bool full)
{

}

void Protocol::before_remove_copy()
{
    is_copy_ = true;
}

void Protocol::lost_msg_detected(uint8_t /*msg_id*/, uint8_t /*expected*/)
{
    set_connection_state(connection_state() | CS_CONNECTED_WITH_LOSSES);
    log_sync_.check();
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

    case Cmd::LOG_PACK_VALUES:
        send_answer(Cmd::LOG_PACK_VALUES, msg_id);
        Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Log_Sync_Values::process_pack, &log_sync_.values_);
        break;
    case Cmd::LOG_PACK_EVENTS:
        Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Log_Sync_Events::process_pack, &log_sync_.events_);
        break;

    case Cmd::SET_MODE:
        send_answer(Cmd::SET_MODE, msg_id);
        apply_parse(data_dev, &Protocol::mode_changed);
        break;
    case Cmd::ADD_STATUS:
        send_answer(Cmd::ADD_STATUS, msg_id);
        apply_parse(data_dev, &Protocol::status_added);
        break;
    case Cmd::REMOVE_STATUS:
        send_answer(Cmd::REMOVE_STATUS, msg_id);
        apply_parse(data_dev, &Protocol::status_removed);
        break;
    case Cmd::SET_DIG_PARAM_VALUES:
        send_answer(Cmd::SET_DIG_PARAM_VALUES, msg_id);
        apply_parse(data_dev, &Protocol::dig_param_values_changed);
        break;

    case Cmd::MODIFY_SCHEME:
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
    qDebug().noquote() << title() << "unprocess answer" << int(msg_id) << int(cmd) << static_cast<Cmd::Command_Type>(cmd) << QDateTime::currentMSecsSinceEpoch();
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
        db()->checkAuth(info, this, device_connection_id);
    }

    bool authenticated = id();

    qDebug().noquote() << title() << "authentication" << info.scheme_name() << "login" << info.login() << ' '
              << info.device_id() << "status:" << (authenticated ? "true" : "false");
    send_answer(Cmd::AUTH, msg_id) << authenticated << device_connection_id;

    if (authenticated)
    {
        work_object()->server_thread_->server()->remove_copy(this);

        structure_sync_.set_modified(modified);
        set_connection_state(CS_CONNECTED_JUST_NOW | (modified ? CS_CONNECTED_MODIFIED : 0));

        send(Cmd::VERSION).answer([this](QIODevice& data_dev) { print_version(data_dev); });
        send(Cmd::TIME_INFO).answer([this](QIODevice& data_dev) { apply_parse(data_dev, &Protocol::set_time_offset); });

        structure_sync_.check(modified);
        log_sync_.init_last_time();
        log_sync_.check();

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

void Protocol::mode_changed(uint32_t /*user_id*/, uint32_t mode_id, uint32_t group_id)
{
    db()->deffered_setMode(name(), mode_id, group_id);
    QMetaObject::invokeMethod(work_object()->dbus_, "dig_mode_item_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *this), Q_ARG(uint32_t, mode_id), Q_ARG(uint32_t, group_id));
}

void Protocol::status_added(uint32_t group_id, uint32_t info_id, const QStringList& args)
{
    // db()->deffered_savestatus(name(), group_id, info_id, args);
    QMetaObject::invokeMethod(work_object()->dbus_, "status_inserted", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *this), Q_ARG(uint32_t, group_id), Q_ARG(uint32_t, info_id), Q_ARG(QStringList, args));
}

void Protocol::status_removed(uint32_t group_id, uint32_t info_id)
{
    // db()->deffered_removestatus(name(), group_id, info_id);
    QMetaObject::invokeMethod(work_object()->dbus_, "status_removed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *this), Q_ARG(uint32_t, group_id), Q_ARG(uint32_t, info_id));
}

void Protocol::dig_param_values_changed(uint32_t /*user_id*/, const QVector<DIG_Param_Value>& pack)
{
    db()->deffered_save_dig_param_value(name(), pack);
    QMetaObject::invokeMethod(work_object()->dbus_, "dig_param_values_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *this), Q_ARG(QVector<DIG_Param_Value>, pack));
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
} // namespace Ver_2_1
} // namespace Das
