#ifndef DAS_DBUS_DBUS_INTERFACE_H
#define DAS_DBUS_DBUS_INTERFACE_H

#include <QObject>

#include <Das/db/dig_status.h>
#include <Das/db/dig_param_value.h>
#include <Das/db/device_item_value.h>
#include <Das/log/log_pack.h>

#include <plus/das/scheme_info.h>
#include <dbus/dbus_common.h>

QT_FORWARD_DECLARE_CLASS(QDBusConnection)
QT_FORWARD_DECLARE_CLASS(QDBusInterface)
QT_FORWARD_DECLARE_CLASS(QDBusServiceWatcher)

namespace Das {
namespace DBus {

template<typename T> struct Non_Void { using type = T; };
template<> struct Non_Void<void> { using type = void*; };

class Handler_Object : public QObject
{
    Q_OBJECT
public:
    virtual void server_down() {}

//    virtual void device_item_values_available(const Scheme_Info& scheme, const QVector<Log_Value_Item>& pack) {}
//    virtual void event_message_available(const Scheme_Info& scheme, const QVector<Log_Event_Item>& event_pack) {}
//    virtual void time_info(const Scheme_Info& scheme, const QTimeZone& tz, qint64 time_offset) {}

//    virtual void process_statuses(const Scheme_Info& scheme, const QVector<DIG_Status>& add, const QVector<DIG_Status>& remove, const QString& db_name);
//    virtual void structure_changed(const Scheme_Info& scheme, const QByteArray& data) {}
//    virtual void dig_param_values_changed(const Scheme_Info& scheme, const QVector<DIG_Param_Value> &pack) {}
//    virtual void dig_mode_changed(const Scheme_Info& scheme, const QVector<DIG_Mode> &pack) {}
//    virtual void status_changed(const Scheme_Info& scheme, const QVector<DIG_Status>& pack) {}
//    virtual void stream_toggled(const Scheme_Info& scheme, uint32_t user_id, uint32_t dev_item_id, bool state);
//    virtual void stream_data(const Scheme_Info& scheme, uint32_t dev_item_id, const QByteArray& data);
    virtual void connect_to(QDBusInterface* iface) { Q_UNUSED(iface); }

    bool is_manual_connect_ = false;
};

class Interface : public QObject
{
    Q_OBJECT
public:
    Interface(Handler_Object* handler,
                   const QString& service_name = DAS_DBUS_DEFAULT_SERVICE_SERVER,
                   const QString& object_path = DAS_DBUS_DEFAULT_OBJECT,
                   const QString& interface_name = DAS_DBUS_DEFAULT_INTERFACE);
    virtual ~Interface();
private:
    void service_registered(const QString &service);
    void service_unregistered(const QString &service);
    void delete_iface();

    void connect_to_interface();
public slots:
    bool is_connected(uint32_t scheme_id);
    uint8_t get_scheme_connection_state(const std::set<uint32_t> &scheme_group_set, uint32_t scheme_id);
    uint8_t get_scheme_connection_state2(uint32_t scheme_id);
    Scheme_Status get_scheme_status(uint32_t scheme_id) const;
    void set_scheme_name(uint32_t scheme_id, uint32_t user_id, const QString& name);
    QVector<Device_Item_Value> get_device_item_values(uint32_t scheme_id) const;
    void send_message_to_scheme(uint32_t scheme_id, uint8_t ws_cmd, uint32_t user_id, const QByteArray& data);

private:
    template<typename Ret_Type,  typename... Args>
    Ret_Type call_iface(const QString& name, const typename Non_Void<Ret_Type>::type ret, Args... args) const;

    template<typename... Args>
    void call_iface_void(const QString& name, Args... args);

    QDBusConnection* bus_;
    QDBusInterface* iface_;
    QDBusServiceWatcher* watcher_;
    Handler_Object* handler_;
    QString service_name_, object_path_, interface_name_;
};

} // namespace DBus
} // namespace Das

#endif // DAS_DBUS_DBUS_INTERFACE_H
