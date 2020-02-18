#ifndef DBUS_OBJECT_H
#define DBUS_OBJECT_H

#include <memory>

#include <QDBusContext>

#include <Das/db/dig_status.h>
#include <Das/db/dig_param_value.h>
#include <Das/log/log_pack.h>
#include <plus/das/scheme_info.h>

#include <dbus/dbus_object_base.h>

namespace Das {
class Worker;
namespace Client {

class Dbus_Object final : public DBus::Object_Base
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DAS_DBUS_DEFAULT_INTERFACE)
public:
    explicit Dbus_Object(Worker* worker,
                         const QString& service_name = DAS_DBUS_DEFAULT_SERVICE_CLIENT,
                         const QString& object_path = DAS_DBUS_DEFAULT_OBJECT);

signals:
    OBJECT_BASE_SIGNALS
public slots:
    bool can_restart(bool stop = false);

    bool is_connected(uint32_t scheme_id) const override;
    uint8_t get_scheme_connection_state(const std::set<uint32_t> &scheme_group_set, uint32_t scheme_id) const override;
    uint8_t get_scheme_connection_state2(uint32_t scheme_id) const override;
    Scheme_Status get_scheme_status(uint32_t scheme_id) const override;
    QVector<Device_Item_Value> get_device_item_values(uint32_t scheme_id) const override;

    void send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& raw_data) override;
    QString get_ip_address(uint32_t scheme_id) const override;
    void write_item_file(uint32_t scheme_id, uint32_t user_id, uint32_t dev_item_id, const QString& file_name, const QString& file_path) override;
private:
    void write_to_item(uint32_t user_id, uint32_t item_id, const QVariant &raw_data);
    void set_dig_param_values(uint32_t user_id, const QVector<DIG_Param_Value>& pack);
    void parse_script_command(uint32_t user_id, const QString &script, QDataStream* data);

    Das::Worker* worker_;
};

} // namespace Client
} // namespace Das

#endif // DBUS_OBJECT_H
