#ifndef DBUS_OBJECT_H
#define DBUS_OBJECT_H

#include <memory>

#include <dbus/dbus_object_base.h>

namespace Helpz {
namespace DTLS {
class Server;
class Server_Node;
}
}

namespace Das {
namespace Server {

class Dbus_Object final : public DBus::Object_Base
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DAS_DBUS_DEFAULT_INTERFACE)
public:
    explicit Dbus_Object(Helpz::DTLS::Server* server,
                         const QString& service_name = DAS_DBUS_DEFAULT_SERVICE_SERVER,
                         const QString& object_path = DAS_DBUS_DEFAULT_OBJECT);
    ~Dbus_Object();

    std::shared_ptr<Helpz::DTLS::Server_Node> find_client(uint32_t scheme_id) const;

signals:
    OBJECT_BASE_SIGNALS
public slots:
    bool is_connected(uint32_t scheme_id) const override;
    uint8_t get_scheme_connection_state(const std::set<uint32_t> &scheme_group_set, uint32_t scheme_id) const override;
    uint8_t get_scheme_connection_state2(uint32_t scheme_id) const override;
    Scheme_Status get_scheme_status(uint32_t scheme_id) const override;
    void set_scheme_name(uint32_t scheme_id, uint32_t user_id, const QString& name);
    QVector<Device_Item_Value> get_device_item_values(uint32_t scheme_id) const override;

    void send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& data) override;
    QString get_ip_address(uint32_t scheme_id) const override;
    void write_item_file(uint32_t scheme_id, uint32_t user_id, uint32_t dev_item_id, const QString& file_name, const QString& file_path) override;
private:
    Helpz::DTLS::Server* server_;
};

} // namespace Server
} // namespace Das

#endif // DBUS_OBJECT_H
