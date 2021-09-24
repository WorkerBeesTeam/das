#ifndef DAS_MODBUS_PLAGIN_BASE_H
#define DAS_MODBUS_PLAGIN_BASE_H

#include <memory>

#include <QLoggingCategory>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QSerialPort>
#include <QTimer>

#include <Das/checker_interface.h>

#include "../plugin_global.h"
#include "config.h"

namespace Das {
namespace Modbus {

Q_DECLARE_LOGGING_CATEGORY(ModbusLog)

struct Modbus_Queue;

class DAS_PLUGIN_SHARED_EXPORT Modbus_Plugin_Base : public QObject, public Checker::Interface
{
    Q_OBJECT
public:
    Modbus_Plugin_Base();
    ~Modbus_Plugin_Base();

    bool check_break_flag() const;
    void clear_break_flag();

    const Config& base_config() const;

    bool checkConnect(Device *dev);

    static int32_t address(Device* dev, bool *ok = nullptr);
    static int32_t unit(Device_Item* item, bool *ok = nullptr);
    static int32_t count(Device_Item* item, int32_t unit);

    // CheckerInterface interface
public:
    virtual void configure(QSettings* settings) override;
    virtual bool check(Device *dev) override;
    virtual void stop() override;
    virtual void write(Device* dev, std::vector<Write_Cache_Item>& items) override;
public slots:
    QStringList available_ports() const;

    void clear_status_cache();

    void write_multivalue(Das::Device *dev, int server_address, int start_address, const QVariantList& data, bool is_coils, bool silent = true);
private slots:
    void read_finished_slot();
    void write_finished_slot();

protected:
    void clear_queue();
    void print_cached(QModbusClient* modbus, int server_address, QModbusDataUnit::RegisterType register_type, QModbusDevice::Error value, const QString& text);
    QModbusClient* check_connection(Device *dev);
    void read(Device *dev, const QVector<Device_Item *>& dev_items);
    void process_queue();
    QVector<quint16> cache_items_to_values(const std::vector<Write_Cache_Item>& items) const;
    void write_pack(QModbusClient* modbus, int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Write_Cache_Item>& items, QModbusReply** reply);
    void write_finished(QModbusClient *modbus, QModbusReply* reply);
    void read_pack(QModbusClient* modbus, int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Device_Item*>& items, QModbusReply** reply);
    void read_finished(QModbusClient *modbus, QModbusReply* reply);

    struct Status_Holder {
        int _server_address;
        QModbusDataUnit::RegisterType _register_type;
        qintptr _modbus;

        bool operator < (const Status_Holder& o) const
        {
            return _modbus < o._modbus
                 || (_modbus == o._modbus
                     && (_server_address < o._server_address
                         || (_server_address == o._server_address && _register_type < o._register_type)));
        }
    };

    typedef std::map<Status_Holder, QModbusDevice::Error> StatusCacheMap;
    StatusCacheMap dev_status_cache_;
private:
    QModbusClient* modbus_by_dev(Device* dev);
    std::shared_ptr<QModbusClient> init_modbus(Device* dev);

    Config dev_config(Device* dev, bool simple = false);

    Config base_config_;

    Modbus_Queue* queue_;

    bool b_break;

    enum Auto_Port { NoAuto, UseFirstUSB, UseFirstTTY };
    int _auto_port = NoAuto;
    std::chrono::milliseconds _line_use_timeout = std::chrono::milliseconds{50};
    std::chrono::system_clock::time_point line_use_last_time_;
    QTimer process_queue_timer_;

    std::map<std::shared_ptr<QModbusClient>, std::vector<Device*>> _dev_map;

    QModbusClient *_last_modbus;
//    std::shared_ptr<QModbusClient> _modbus;
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_PLAGIN_BASE_H
