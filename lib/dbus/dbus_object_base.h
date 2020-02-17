#ifndef DBUS_OBJECT_BASE_H
#define DBUS_OBJECT_BASE_H

#include <QDBusContext>

#include <Das/db/dig_status.h>
#include <Das/db/dig_param_value.h>
#include <Das/db/device_item_value.h>
#include <Das/log/log_pack.h>
#include <plus/das/scheme_info.h>

#include <dbus/dbus_common.h>

namespace Das {
namespace DBus {

/*
 * Эти сигналы используются и эдентичны как в клиенте так и на сервере,
 * но засунуть их в Object_Base нельзя по тому что там нельзя использовать
 * макрос Q_OBJECT, по тому что иначе не работает DBus
 */
#define OBJECT_BASE_SIGNALS \
    void connection_state_changed(const Scheme_Info& scheme, uint8_t state); \
    void device_item_values_available(const Scheme_Info& scheme, const QVector<Log_Value_Item>& pack); \
    void event_message_available(const Scheme_Info& scheme, const QVector<Log_Event_Item>& event_pack); \
    void time_info(const Scheme_Info& scheme, const QTimeZone& tz, qint64 time_offset); \
    void structure_changed(const Scheme_Info& scheme, const QByteArray& data); \
    void dig_param_values_changed(const Scheme_Info& scheme, const QVector<DIG_Param_Value> &pack); \
    void status_changed(const Scheme_Info& scheme, const QVector<DIG_Status>& pack); \
    void dig_mode_changed(const Scheme_Info& scheme, const QVector<DIG_Mode> &pack);

class Object_Base : public QObject, protected QDBusContext
{
    // Строки ниже нельзя расскоментировать
//    Q_OBJECT
//    Q_CLASSINFO("D-Bus Interface", DAS_DBUS_DEFAULT_INTERFACE)
public:
    explicit Object_Base(const QString& service_name, const QString& object_path = DAS_DBUS_DEFAULT_OBJECT);
    virtual ~Object_Base();

//signals:
//public slots:
    virtual bool is_connected(uint32_t scheme_id) const = 0;
    virtual uint8_t get_scheme_connection_state(const std::set<uint32_t> &scheme_group_set, uint32_t scheme_id) const = 0;
    virtual uint8_t get_scheme_connection_state2(uint32_t scheme_id) const = 0;
    virtual Scheme_Status get_scheme_status(uint32_t scheme_id) const = 0;
    virtual QVector<Device_Item_Value> get_device_item_values(uint32_t scheme_id) const = 0;

    virtual void send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& data) = 0;
    virtual QString get_ip_address(uint32_t scheme_id) const = 0;
    virtual void write_item_file(uint32_t scheme_id, uint32_t user_id, uint32_t dev_item_id, const QString& file_name, const QString& file_path) = 0;

protected:
    uint16_t cmd_from_web_command(quint8 cmd, int proto_version = 202) const;

private:
    QString service_name_, object_path_;
};

} // namespace DBus
} // namespace Das

#endif // DBUS_OBJECT_H
