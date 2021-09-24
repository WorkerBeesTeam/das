#ifndef DAS_MODBUS_CONTROLLER_H
#define DAS_MODBUS_CONTROLLER_H

#include <memory>
#include <vector>
#include <mutex>

#include <QModbusDevice>

#include <Das/device.h>
#include <Das/write_cache_item.h>

#include "modbus_pack_queue.h"
#include "config.h"

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Das {
namespace Modbus {

struct Pack_Queue;

class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(const Config& config);
    ~Controller();

    const Config& config() const;

    bool read(Device* dev);
    void write(std::vector<Write_Cache_Item>& items);
signals:
public slots:
    void set_line_use_timeout(int msec);
    void write_multivalue(Das::Device *dev, int server_address, int start_address, const QVariantList& data, bool is_coils, bool silent = true);

private slots:
    void process_error(QModbusDevice::Error e);
    void check_state(QModbusDevice::State state);
    void next();

    void read_finished_slot();
    void write_finished_slot();

private:
    bool check_connection();

    QVector<quint16> cache_items_to_values(const std::vector<Write_Cache_Item>& items) const;
    void write_pack(int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Write_Cache_Item>& items, QModbusReply** reply);
    void write_finished(QModbusReply* reply);
    void read_pack(int server_address, QModbusDataUnit::RegisterType register_type, int start_address, const std::vector<Device_Item*>& items, QModbusReply** reply);
    void read_finished(QModbusReply* reply);

    void init();
    void deinit();

    const Config _config;

    std::shared_ptr<QModbusClient> _modbus;

    std::chrono::milliseconds _line_use_timeout = std::chrono::milliseconds{50};
    std::chrono::system_clock::time_point _line_use_last_time;
    QTimer* _process_queue_timer;

    Pack_Queue _queue;
    std::mutex _mutex;

    friend class Thread;
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_CONTROLLER_H
