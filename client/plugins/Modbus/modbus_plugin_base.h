#ifndef DAS_MODBUS_PLAGIN_BASE_H
#define DAS_MODBUS_PLAGIN_BASE_H

#include <memory>

#include <QLoggingCategory>
#include <QModbusRtuSerialMaster>
#include <QSerialPort>
#include <QTimer>

#include <Das/checkerinterface.h>

#include "../plugin_global.h"
#include "config.h"

namespace Das {
namespace Modbus {

Q_DECLARE_LOGGING_CATEGORY(ModbusLog)

struct Modbus_Queue;

class DAS_PLUGIN_SHARED_EXPORT Modbus_Plugin_Base : public QModbusRtuSerialMaster, public Checker_Interface
{
    Q_OBJECT
public:
    Modbus_Plugin_Base();
    ~Modbus_Plugin_Base();

    bool check_break_flag() const;
    void clear_break_flag();

    const Config& config() const;

    bool checkConnect();

    static int32_t address(Device* dev, bool *ok = nullptr);
    static int32_t unit(Device_Item* item, bool *ok = nullptr);

    // CheckerInterface interface
public:
    virtual void configure(QSettings* settings, Scheme*) override;
    virtual bool check(Device *dev) override;
    virtual void stop() override;
    virtual void write(std::vector<Write_Cache_Item>& items) override;
public slots:
    QStringList available_ports() const;

    void clear_status_cache();
private slots:
    void write_finished_slot();
    void read_finished_slot();
protected:
    void clear_queue();
    void print_cached(int server_address, QModbusDataUnit::RegisterType register_type, Error value, const QString& text);
    bool reconnect();
    void read(const QVector<Device_Item *>& dev_items);
    void process_queue();
    QVector<quint16> cache_items_to_values(const std::vector<Write_Cache_Item>& items) const;
    void write_pack(int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Write_Cache_Item>& items, QModbusReply** reply);
    void write_finished(QModbusReply* reply);
    void read_pack(int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Device_Item*>& items, QModbusReply** reply);
    void read_finished(QModbusReply* reply);

    typedef std::map<std::pair<int, QModbusDataUnit::RegisterType>, QModbusDevice::Error> StatusCacheMap;
    StatusCacheMap dev_status_cache_;
private:
    Config config_;

    Modbus_Queue* queue_;

    bool b_break, is_port_name_in_config_;
    std::chrono::system_clock::time_point line_use_last_time_;
    QTimer process_queue_timer_;
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_PLAGIN_BASE_H
