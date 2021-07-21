#include <QtDBus>

#include <Helpz/dtls_server.h>

#include <Das/commands.h>

#include "log_saver/log_saver_manager.h"
#include "server_protocol.h"
#include "dbus_object.h"

namespace Das {
namespace Server {

Dbus_Object* Dbus_Object::_ptr = nullptr;
Dbus_Object* Dbus_Object::instance() { return _ptr; }

Dbus_Object::Dbus_Object(Helpz::DTLS::Server* server, const QString& service_name, const QString& object_path) :
    DBus::Object_Base(service_name, object_path),
    server_(server),
    _is_sync_enabled(true)
{
    _ptr = this;
}

Dbus_Object::~Dbus_Object()
{
    _ptr = nullptr;
}

bool Dbus_Object::is_sync_enabled() const
{
    return _is_sync_enabled;
}

void Dbus_Object::set_server(Helpz::DTLS::Server *server)
{
    server_ = server;
}

std::shared_ptr<Helpz::DTLS::Server_Node> Dbus_Object::find_client(uint32_t scheme_id) const
{
    if (!server_)
        return {};
    return server_->find_client([scheme_id](const Helpz::Net::Protocol* protocol) -> bool
    {
        auto p = static_cast<const Protocol_Base*>(protocol);
        return p->id() == scheme_id;
    });
}

bool Dbus_Object::is_connected(uint32_t scheme_id) const
{
    return find_client(scheme_id).operator bool();
}

uint8_t Dbus_Object::get_scheme_connection_state(const std::set<uint32_t>& scheme_group_set, uint32_t scheme_id) const
{
    if (!server_)
        return CS_DISCONNECTED;

    std::shared_ptr<Helpz::DTLS::Server_Node> node = server_->find_client([scheme_id, &scheme_group_set](const Helpz::Net::Protocol* protocol) -> bool
    {
        auto p = static_cast<const Protocol_Base*>(protocol);
        return p->id() == scheme_id && p->check_scheme_groups(scheme_group_set);
    });

    if (node)
    {
        std::shared_ptr<Protocol_Base> p = std::static_pointer_cast<Protocol_Base>(node->protocol());
        if (p)
        {
            uint8_t state = p->connection_state();
            if ((state & ~CS_FLAGS) == CS_CONNECTED && (state & CS_CONNECTED_WITH_LOSSES))
                p->synchronize();
            return state;
        }
    }
    return CS_DISCONNECTED;
}

uint8_t Dbus_Object::get_scheme_connection_state2(uint32_t scheme_id) const
{
    std::shared_ptr<Helpz::DTLS::Server_Node> node = find_client(scheme_id);
    if (node)
    {
        auto p = std::static_pointer_cast<Protocol_Base>(node->protocol());
        if (p)
            return p->connection_state();
    }
    return CS_DISCONNECTED;
}

Scheme_Status Dbus_Object::get_scheme_status(uint32_t scheme_id) const
{
    Scheme_Status scheme_status{CS_DISCONNECTED, Log_Saver::instance()->get_statuses(scheme_id)};

    std::shared_ptr<Helpz::DTLS::Server_Node> node = find_client(scheme_id);
    if (node)
    {
        std::shared_ptr<Protocol_Base> p = std::static_pointer_cast<Protocol_Base>(node->protocol());
        if (p)
            scheme_status.connection_state_ = p->connection_state();
    }
    return scheme_status;
}

Scheme_Time_Info Dbus_Object::get_time_info(uint32_t scheme_id) const
{
    std::shared_ptr<Helpz::DTLS::Server_Node> node = find_client(scheme_id);
    if (node)
    {
        std::shared_ptr<Protocol_Base> p = std::static_pointer_cast<Protocol_Base>(node->protocol());
        if (p)
        {
            Protocol_Base::TimeInfo ti = p->time();
            auto dt = QDateTime::currentDateTimeUtc();
            dt.addMSecs(ti.offset);
            return { ti.zone.offsetFromUtc(dt), dt.toMSecsSinceEpoch(), ti.zone.id().constData() };
        }
    }
    return { 0, 0 };
}

void Dbus_Object::set_scheme_name(uint32_t scheme_id, uint32_t user_id, const QString &name)
{
    std::shared_ptr<Helpz::DTLS::Server_Node> node = find_client(scheme_id);
    if (node)
    {
        std::shared_ptr<Ver::Server::Protocol> proto = std::dynamic_pointer_cast<Ver::Server::Protocol>(node->protocol());
        if (proto)
            return proto->set_scheme_name(user_id, name);
    }
}

QVector<Device_Item_Value> Dbus_Object::get_device_item_values(uint32_t scheme_id) const
{
    return Log_Saver::instance()->get_devitem_values(scheme_id);
}

QVector<Device_Item_Value> Dbus_Object::get_device_item_cached_values(uint32_t scheme_id) const
{
    return Log_Saver::instance()->get_cache_data<Log_Value_Item>(scheme_id);
}

void Dbus_Object::fill_log_value_layers()
{
    Log_Saver::instance()->fill_log_value_layers();
}

void Dbus_Object::organize_log_partition()
{
    Log_Saver::instance()->organize_log_partition();
}

void Dbus_Object::send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& data)
{
    auto node = find_client(scheme_id);
    if (node)
    {
        std::shared_ptr<Protocol_Base> scheme = std::static_pointer_cast<Protocol_Base>(node->protocol());

        uint16_t cmd = cmd_from_web_command(ws_cmd, scheme->protocol_version());
        if (cmd > 0)
        {
            Helpz::Net::Protocol_Sender sender = std::move(scheme->send(cmd));
            sender << user_id;
            sender.writeRawData(data.constData(), data.size());
        }
    }
}

QString Dbus_Object::get_ip_address(uint32_t scheme_id) const
{
    auto node = find_client(scheme_id);
    if (node)
        return QString::fromStdString(node->address());
    return {};
}

void Dbus_Object::write_item_file(uint32_t scheme_id, uint32_t user_id, uint32_t dev_item_id, const QString &file_name, const QString &file_path)
{
    qInfo() << "Dbus_Object::write_item_file" << "scheme_id" << scheme_id << "user_id" << user_id << "dev_item_id" << dev_item_id << "file_name" << file_name << "file_path" << file_path;
    auto node = find_client(scheme_id);
    if (node)
    {
        auto proto = std::static_pointer_cast<Protocol_Base>(node->protocol());
        if (proto)
            proto->send_file(user_id, dev_item_id, file_name, file_path);
    }
}

void Dbus_Object::start_stream(uint32_t scheme_id, uint32_t user_id, uint32_t dev_item_id, const QString &url)
{
    auto node = find_client(scheme_id);
    if (node)
    {
        auto proto = std::static_pointer_cast<Protocol_Base>(node->protocol());
        if (proto)
            proto->send_start_stream(user_id, dev_item_id, url);
    }
}

bool Dbus_Object::ping()
{
    return true;
}

void Dbus_Object::toggle_sync(bool state)
{
    _is_sync_enabled = state;
}

} // namespace Server
} // namespace Das
