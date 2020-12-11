#include <QTemporaryFile>

#include <Helpz/dtls_version.h>
#include <Helpz/net_version.h>
#include <Helpz/db_version.h>
#include <Helpz/srv_version.h>
#include <Helpz/dtls_client_node.h>

#include <Das/commands.h>
#include <Das/lib.h>

#include "worker.h"
#include "client_protocol_latest.h"

namespace Das {
namespace Ver {
namespace Client {

Config::Config(uint32_t start_stream_timeout_ms) :
    _start_stream_timeout_ms{start_stream_timeout_ms}
{
}

/*static*/ Config &Config::instance()
{
    static Config conf;
    return conf;
}

// ---------------------------------------------

Protocol::Protocol(Worker *worker) :
    Protocol_Base{worker},
    prj_(worker->prj()),
    log_sender_(this),
    structure_sync_(worker->db_pending(), this)
{
    qRegisterMetaType<QVector<DIG_Param_Value>>("QVector<DIG_Param_Value>");

    /*
    qRegisterMetaType<Code_Item>("Code_Item");
    qRegisterMetaType<std::vector<uint>>("std::vector<uint>");
    connect(this, &Protocol::setCode, worker, &Worker::setCode, Qt::BlockingQueuedConnection);
//    connect(this, &Protocol::lostValues, worker, &Worker::sendLostValues, Qt::QueuedConnection);
    connect(this, &Protocol::getServerInfo, worker->prj->ptr(), &Scheme::dumpInfoToStream, Qt::DirectConnection);
    connect(this, &Protocol::setServerInfo, worker->prj->ptr(), &Scheme::initFromStream, Qt::BlockingQueuedConnection);
    connect(this, &Protocol::saveServerInfo, worker->database(), &DB::saveScheme, Qt::BlockingQueuedConnection);
    */
}

Protocol::~Protocol()
{
}

Log_Sender& Protocol::log_sender()
{
    return log_sender_;
}

Structure_Synchronizer& Protocol::structure_sync()
{
    return structure_sync_;
}

//void Protocol::send_mode(const DIG_Mode& mode)
//{
//    send(Cmd::SET_MODE).timeout(nullptr, std::chrono::seconds(16), std::chrono::seconds(5)) << mode;
//}

//void Protocol::send_status_changed(const DIG_Status &status)
//{
//    send(Cmd::CHANGE_STATUS).timeout([=]()
//    {
//        qCDebug(NetClientLog) << "Send status timeout. Group:" << status.group_id() << "info:" << status.status_id()
//                              << status.args() << "is_removed:" << status.is_removed();
//        send(Cmd::GROUP_STATUSES) << get_group_statuses();
//    }, std::chrono::seconds(5), std::chrono::milliseconds(1500)) << status;
//}

//void Protocol::send_dig_param_values(uint32_t user_id, const QVector<DIG_Param_Value> &pack)
//{
//    send(Cmd::SET_DIG_PARAM_VALUES).timeout(nullptr, std::chrono::seconds(16), std::chrono::seconds(5)) << user_id << pack;
//}

QVector<DIG_Status> Protocol::get_group_statuses() const
{
    QVector<DIG_Status> statuses;
    QMetaObject::invokeMethod(worker()->prj(), "get_group_statuses", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVector<DIG_Status>, statuses));
    return statuses;
}

void Protocol::restart(uint32_t user_id)
{
    QMetaObject::invokeMethod(worker(), "restart_service_object", Qt::QueuedConnection, Q_ARG(uint32_t, user_id));
}

void Protocol::write_to_item(uint32_t user_id, uint32_t item_id, const QVariant& raw_data)
{
    QMetaObject::invokeMethod(worker()->prj(), "write_to_item", Qt::QueuedConnection,
                              Q_ARG(uint32_t, user_id), Q_ARG(uint32_t, item_id), Q_ARG(QVariant, raw_data));
}

void Protocol::set_dig_param_values(uint32_t user_id, const QVector<DB::DIG_Param_Value_Base>& pack)
{
    if (!pack.empty())
    {
        QMetaObject::invokeMethod(worker()->prj(), "set_dig_param_values", Qt::QueuedConnection,
                                  Q_ARG(uint32_t, user_id), Q_ARG(QVector<DB::DIG_Param_Value_Base>, pack));
    }
}

void Protocol::exec_script_command(uint32_t user_id, const QString& script, bool is_function, const QVariantList& arguments)
{
    QMetaObject::invokeMethod(worker()->prj(), "console", Qt::QueuedConnection,
                              Q_ARG(uint32_t, user_id), Q_ARG(QString, script), Q_ARG(bool, is_function), Q_ARG(QVariantList, arguments));
}

void Protocol::ready_write()
{
    auto ctrl = std::dynamic_pointer_cast<const Helpz::DTLS::Client_Node>(writer());
    if (ctrl)
        qCDebug(NetClientLog) << "Connected. Server choose protocol:" << ctrl->application_protocol().c_str();

    start_authentication();
}

struct Client_Bad_Fix : public Bad_Fix
{
    Structure_Synchronizer_Base* get_structure_synchronizer() override
    {
        scheme_ = std::dynamic_pointer_cast<Protocol>(node_->protocol());
        if (scheme_)
            return &scheme_->structure_sync();
        return nullptr;
    }

    std::shared_ptr<Helpz::DTLS::Node> node_;
    std::shared_ptr<Protocol> scheme_;
};

void Protocol::process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    switch (cmd)
    {

    case Cmd::NO_AUTH:                  start_authentication(); break;
    case Cmd::VERSION:                  send_version(msg_id);   break;
    case Cmd::TIME_INFO:                send_time_info(msg_id); break;

    case Cmd::RESTART:                  apply_parse(data_dev, &Protocol::restart);                          break;
    case Cmd::WRITE_TO_ITEM:            apply_parse(data_dev, &Protocol::write_to_item);                    break;
    case Cmd::WRITE_TO_ITEM_FILE:       process_item_file(data_dev);                                        break;
    case Cmd::SET_MODE:                 Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Worker::set_mode, worker()); break;
    case Cmd::SET_DIG_PARAM_VALUES:     apply_parse(data_dev, &Protocol::set_dig_param_values);             break;
    case Cmd::EXEC_SCRIPT_COMMAND:      apply_parse(data_dev, &Protocol::parse_script_command, &data_dev);  break;
    case Cmd::STREAM_START:             apply_parse(data_dev, &Protocol::start_stream, msg_id);             break;

    case Cmd::GET_SCHEME:               Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Structure_Synchronizer::send_scheme_structure, &structure_sync_, msg_id, &data_dev); break;
    case Cmd::MODIFY_SCHEME:
    {
        send_answer(Cmd::MODIFY_SCHEME, msg_id);

        auto node = std::static_pointer_cast<Helpz::DTLS::Node>(writer());
        auto bad_fix_func = [node]() -> std::shared_ptr<Bad_Fix>
        {
            std::shared_ptr<Client_Bad_Fix> bad_fix(new Client_Bad_Fix);
            bad_fix->node_ = node;

            return std::static_pointer_cast<Bad_Fix>(bad_fix);
        };

        Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Structure_Synchronizer::process_modify_message, &structure_sync_,
                           &data_dev, DB::Schemed_Model::default_scheme_id(), bad_fix_func);
        break;
    }

    case Cmd::LOG_DATA_REQUEST:         Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Log_Sender::send_data, &log_sender_, msg_id);    break;

    case Cmd::DEVICE_ITEM_VALUES:
    {
        QVector<Device_Item_Value> value_vect;
        QMetaObject::invokeMethod(worker()->log_timer_thread_->ptr(), "get_unsaved_values", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QVector<Device_Item_Value>, value_vect));
        qDebug(NetClientLog) << "DEVICE_ITEM_VALUES" << value_vect.size() << "msg" << msg_id;
        send_answer(Cmd::DEVICE_ITEM_VALUES, msg_id) << value_vect;
        break;
    }
    case Cmd::GROUP_STATUSES:
        send_answer(Cmd::GROUP_STATUSES, msg_id) << get_group_statuses();
        break;

    case Cmd::SET_SCHEME_NAME:
        send_answer(Cmd::SET_SCHEME_NAME, msg_id);
        Helpz::apply_parse(data_dev, DATASTREAM_VERSION, &Worker::set_scheme_name, worker());
        break;

    default:
        if (cmd >= Helpz::Net::Cmd::USER_COMMAND)
        {
            if (!data_dev.isOpen())
                data_dev.open(QIODevice::ReadOnly);
            qCCritical(NetClientLog) << "UNKNOWN MESSAGE" << int(cmd) << "id:" << msg_id << "size:" << data_dev.size() << data_dev.bytesAvailable()
                                     << "hex:" << data_dev.pos() << data_dev.readAll().toHex();
        }
        break;
    }
}

void Protocol::process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice& /*data_dev*/)
{
    qCWarning(NetClientLog) << "unprocess answer" << msg_id << int(cmd);
}

void Protocol::parse_script_command(uint32_t user_id, const QString& script, QIODevice* data_dev)
{
    QVariantList arguments;
    bool is_function = data_dev->bytesAvailable();
    if (is_function)
    {
        parse_out(*data_dev, arguments);
    }
    exec_script_command(user_id, script, is_function, arguments);
}

void Protocol::start_stream(uint32_t user_id, uint32_t dev_item_id, const QString& url, uint8_t msg_id)
{
    send_answer(Cmd::STREAM_START, msg_id) << worker()->start_stream(user_id, dev_item_id, url);
}

void Protocol::process_item_file(QIODevice& data_dev)
{
    if (!data_dev.isOpen())
    {
        if (!data_dev.open(QIODevice::ReadOnly))
        {
            qCCritical(NetClientLog) << "Fail to open data_dev in process_item_file";
            return;
        }
    }

    QString file_name;
    auto file = dynamic_cast<QTemporaryFile*>(&data_dev);
    if (file)
    {
        file->setAutoRemove(false);
        file_name = file->fileName();
        file->close();
    }
    else
    {
        QTemporaryFile t_file;
        if (t_file.open())
        {
            t_file.setAutoRemove(false);
            file_name = t_file.fileName();

            char buffer[1024];
            int length;
            while ((length = data_dev.read(buffer, sizeof(buffer))) > 0)
                t_file.write(buffer, length);
            t_file.close();
        }
    }

    if (file_name.isEmpty())
        qCWarning(NetClientLog) << "Attempt to write broken file. Size:" << data_dev.size();
    else
    {
        QMetaObject::invokeMethod(worker()->prj(), "write_to_item_file", Qt::QueuedConnection, Q_ARG(QString, file_name));
    }
}

void Protocol::start_authentication()
{
    send(Cmd::AUTH).answer([this](QIODevice& data_dev)
    {
        apply_parse(data_dev, &Protocol::process_authentication);
    }).timeout([]() {
        std::cout << "Authentication timeout" << std::endl;
    }, std::chrono::seconds(45), std::chrono::seconds(15)) << Config::instance()._auth << structure_sync_.modified();
}

void Protocol::process_authentication(bool authorized, const QUuid& connection_id)
{
    // TODO: В случае неудачи, нужно отключаться и переподключаться через 15 минут.
    qCDebug(NetClientLog) << "Authentication status:" << authorized;
    if (authorized)
    {
        if (connection_id != Config::instance()._auth.using_key())
        {
            Worker::store_connection_id(connection_id);
        }
    }
    else
        worker()->close_net_client();
}

void Protocol::send_version(uint8_t msg_id)
{
    send_answer(Cmd::VERSION, msg_id)
            << Helpz::DTLS::ver_major() << Helpz::DTLS::ver_minor() << Helpz::DTLS::ver_build()
            << Helpz::Net::ver_major() << Helpz::Net::ver_minor() << Helpz::Net::ver_build()
            << Helpz::DB::ver_major() << Helpz::DB::ver_minor() << Helpz::DB::ver_build()
            << Helpz::Service::ver_major() << Helpz::Service::ver_minor() << Helpz::Service::ver_build()
            << Lib::ver_major() << Lib::ver_minor() << Lib::ver_build()
            << (quint8)VER_MJ << (quint8)VER_MN << (int)VER_B;
}

void Protocol::send_time_info(uint8_t msg_id)
{
    auto dt = QDateTime::currentDateTime();
    send_answer(Cmd::TIME_INFO, msg_id) << dt << dt.timeZone();
}

// -----------------------

#if 0
#include <botan-2/botan/parsing.h>

class Client : public QObject, public Helpz::Net::Protocol
{
    Q_OBJECT
public:
    Client(Worker *worker, const QString& hostname, quint16 port, const QString& login, const QString& password, const QUuid& device, int checkServerInterval) :
        Helpz::DTLS::Protocol(Botan::split_on("das/1.1,das/1.0", ','),
                              qApp->applicationDirPath() + "/tls_policy.conf", hostname, port, checkServerInterval),
        m_login(login), m_password(password), m_device(device), m_import_config(false),
        worker(worker)
    {
        if (canConnect())
            init_client();
    }
    const QUuid& device() const { return m_device; }
    const QString& username() const { return m_login; }

//    bool canConnect() const override { return !(/*m_device.isNull() || */m_login.isEmpty() || m_password.isEmpty()); }
signals:
    void getServerInfo(QIODevice* dev) const;
    void setServerInfo(QIODevice* dev, QVector<DIG_Param_Type_Item> *param_items_out, bool* = nullptr);
    void saveServerInfo(const QVector<DIG_Param_Type_Item>& param_values, Scheme *scheme);

    bool setCode(const Code_Item&);
public slots:
    void setImportFlag(bool import_config)
    {
        if (m_import_config != import_config)
        {
            m_import_config = import_config;
            init_client();
        }
    }
    void refreshAuth(const QUuid& devive_uuid, const QString& username, const QString& password)
    {
        close_connection();
        setDevice(devive_uuid);
        setLogin(username);
        setPassword(password);
        if (canConnect())
            init_client();
    }
    void importDevice(const QUuid& devive_uuid)
    {
        /**
     *   1. Установить, подключиться, получить все данные
     *   2. Отключится
     *   3. Импортировать полученные данные
     **/

        Helpz::Net::Waiter w(cmdGetServerInfo, wait_map);
        if (!w) return;

        send(cmdGetServerInfo) << m_login << m_password << devive_uuid;
        w.helper->wait();
    }
    QUuid createDevice(const QString& name, const QString &latin, const QString& description)
    {
        if (name.isEmpty() || latin.isEmpty())
            return {};

        Helpz::Net::Waiter w(cmdCreateDevice, wait_map);
        if (w)
        {
            send(cmdCreateDevice) << m_login << m_password << name << latin << description;
            if (w.helper->wait())
                return w.helper->result().toUuid();
        }

        return {};
    }

    QVector<QPair<QUuid, QString>> getUserDevices()
    {
        Helpz::Net::Waiter w(cmdAuth, wait_map);
        if (!w)
            return lastUserDevices;

        send(cmdAuth) << m_login << m_password << QUuid();
        bool res = w.helper->wait();
        if (res)
        {
            res = w.helper->result().toBool();

            if (res)
            {
                // TODO: What?
            }
        }
        return lastUserDevices;
    }
protected:
    void process_message(uint8_t msg_id, uint16_t cmd, QIODevice& data_dev) override
    {
        switch(cmd)
        {
        case cmdAuth:
            if (!authorized)
            {
                parse_out(data_dev, lastUserDevices);
                auto wait_it = wait_map.find(cmdAuth);
                if (wait_it != wait_map.cend())
                    wait_it->second->finish(authorized);

                // TODO: Disconnect?
                // close_connection();
                break;
            }

            if (m_import_config)
                send(cmdGetServerInfo);
            break;

        case cmdCreateDevice:
        {
            auto wait_it = wait_map.find(cmdCreateDevice);
            if (wait_it != wait_map.cend())
            {
                QUuid uuid;
                parse_out(data_dev, uuid);
                wait_it->second->finish(uuid);
            }
            break;
        }
        case cmdServerInfo:
        {
            close_connection();
            QVector<DIG_Param_Type_Item> param_values;
            emit setServerInfo(&msg, &param_values);
            emit saveServerInfo(param_values, worker->prj->ptr());

            auto wait_it = wait_map.find(cmdGetServerInfo);
            if (wait_it != wait_map.cend())
                wait_it->second->finish();
            break;
        }
        }
    }
private:
    bool m_import_config;

    Helpz::Net::WaiterMap wait_map;
    QVector<QPair<QUuid, QString>> lastUserDevices;
};
#endif

} // namespace Client
} // namespace Ver
} // namespace Das
