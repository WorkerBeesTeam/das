#include <QtDBus>
#include <QFileInfo>

#include <Das/commands.h>

#include "dbus_object.h"
#include "worker.h"

namespace Das {
namespace Client {

Dbus_Object::Dbus_Object(Worker* worker, const QString& service_name, const QString& object_path) :
    DBus::Object_Base(service_name, object_path),
    worker_(worker)
{
}

bool Dbus_Object::can_restart(bool stop)
{
    bool res;
    QMetaObject::invokeMethod(worker_->prj(), "can_restart", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, res), Q_ARG(bool, stop));
    return res;
}

bool Dbus_Object::is_connected(uint32_t /*scheme_id*/) const
{
    return true;
}

uint8_t Dbus_Object::get_scheme_connection_state(const std::set<uint32_t>& /*scheme_group_set*/, uint32_t /*scheme_id*/) const
{
    return CS_CONNECTED;
}

uint8_t Dbus_Object::get_scheme_connection_state2(uint32_t /*scheme_id*/) const
{
    return CS_CONNECTED;
}

Scheme_Status Dbus_Object::get_scheme_status(uint32_t /*scheme_id*/) const
{
    QVector<DIG_Status> status_vect;
    QMetaObject::invokeMethod(worker_->prj(), "get_group_statuses", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVector<DIG_Status>, status_vect));

    Scheme_Status scheme_status{CS_CONNECTED, {}};
    for (const DIG_Status& status: status_vect)
        scheme_status.status_set_.insert(status);
    return scheme_status;
}

void Dbus_Object::set_scheme_name(uint32_t scheme_id, uint32_t user_id, const QString &name)
{
    if (scheme_id == DB::Schemed_Model::default_scheme_id())
        worker_->set_scheme_name(user_id, name);
}

QVector<Device_Item_Value> Dbus_Object::get_device_item_values(uint32_t /*scheme_id*/) const
{
    QVector<Device_Item_Value> values;
    QMetaObject::invokeMethod(worker_->prj(), "get_device_item_values", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVector<Device_Item_Value>, values));
    return values;
}

QVector<Device_Item_Value> Dbus_Object::get_device_item_cached_values(uint32_t scheme_id) const
{
    return get_device_item_values(scheme_id);
}

void Dbus_Object::send_message_to_scheme(uint32_t /*scheme_id*/, uint8_t ws_cmd, uint32_t user_id, const QByteArray& raw_data)
{
    QByteArray data(4, Qt::Uninitialized);
    data += raw_data;
    QDataStream ds(&data, QIODevice::ReadWrite);
    ds.setVersion(Helpz::Net::Protocol::DATASTREAM_VERSION);
    ds << user_id;
    ds.device()->seek(0);

    try
    {
        switch (ws_cmd)
        {
        case WS_WRITE_TO_DEV_ITEM:        Helpz::apply_parse(ds, &Dbus_Object::write_to_item, this); break;
        case WS_CHANGE_DIG_MODE:          Helpz::apply_parse(ds, &Worker::set_mode, worker_); break;
        case WS_CHANGE_DIG_PARAM_VALUES:  Helpz::apply_parse(ds, &Dbus_Object::set_dig_param_values, this); break;
        case WS_EXEC_SCRIPT:              Helpz::apply_parse(ds, &Dbus_Object::parse_script_command, this, &ds); break;
        case WS_RESTART:                  worker_->restart_service_object(user_id); break;
        case WS_STRUCT_MODIFY:
            Helpz::apply_parse(ds, &Worker_Structure_Synchronizer::process_modify_message, worker_->structure_sync(),
                               ds.device(), DB::Schemed_Model::default_scheme_id(), nullptr);
            break;

        default:
            qCWarning(DBus_log) << "Dbus_Object::send_message_to_scheme Unknown Message:" << (WebSockCmd)ws_cmd;
            break;
        }
    }
    catch(const std::exception& e)
    {
        qCCritical(DBus_log) << "Dbus_Object::send_message_to_scheme:" << e.what();
    }
    catch(...)
    {
        qCCritical(DBus_log) << "Dbus_Object::send_message_to_scheme unknown exception";
    }
}

QString Dbus_Object::get_ip_address(uint32_t /*scheme_id*/) const
{
    return {};
}

void Dbus_Object::write_item_file(uint32_t /*scheme_id*/, uint32_t user_id, uint32_t dev_item_id, const QString &file_name, const QString &file_path)
{
    qCInfo(DBus_log).nospace() << user_id << "|Dbus_Object::write_item_file item_id " << dev_item_id << " file_name " << file_name << " file_path " << file_path;
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(DBus_log).noquote() << "Can't open file:" << file.errorString();
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(&file))
    {
        qCWarning(DBus_log).noquote() << "Can't get file hash";
        return;
    }

    QByteArray item_value_data;
    QDataStream ds(&item_value_data, QIODevice::WriteOnly);
    ds << file_name << hash.result();

    QVariant raw_data = QVariant::fromValue(item_value_data);

    write_to_item(0, dev_item_id, raw_data);
    QMetaObject::invokeMethod(worker_->prj(), "write_to_item_file", Qt::QueuedConnection, Q_ARG(QString, file_path));
}

void Dbus_Object::write_to_item(uint32_t user_id, uint32_t item_id, const QVariant& raw_data)
{
    QMetaObject::invokeMethod(worker_->prj(), "write_to_item", Qt::QueuedConnection,
                              Q_ARG(uint32_t, user_id), Q_ARG(uint32_t, item_id), Q_ARG(QVariant, raw_data));

}

void Dbus_Object::set_dig_param_values(uint32_t user_id, const QVector<DIG_Param_Value>& pack)
{
    if (!pack.empty())
    {
        QMetaObject::invokeMethod(worker_->prj(), "set_dig_param_values", Qt::QueuedConnection, Q_ARG(uint32_t, user_id), Q_ARG(QVector<DIG_Param_Value>, pack));
    }
}

void Dbus_Object::parse_script_command(uint32_t user_id, const QString& script, QDataStream* data)
{
    QVariantList arguments;
    bool is_function = data->device()->bytesAvailable();
    if (is_function)
    {
        Helpz::parse_out(*data, arguments);
    }
    worker_->prj()->console(user_id, script, is_function, arguments);
}

} // namespace Client
} // namespace Das
