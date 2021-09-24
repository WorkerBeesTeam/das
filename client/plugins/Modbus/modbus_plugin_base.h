#ifndef DAS_MODBUS_PLAGIN_BASE_H
#define DAS_MODBUS_PLAGIN_BASE_H

#include <memory>

#include <QLoggingCategory>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QSerialPort>
#include <QTimer>

#include <Das/checker_interface.h>

#include "modbus_log.h"
#include "../plugin_global.h"
#include "config.h"

namespace Das {
namespace Modbus {

struct Pack_Queue;
class Thread;
class Controller;

class DAS_PLUGIN_SHARED_EXPORT Modbus_Plugin_Base : public QObject, public Checker::Interface
{
    Q_OBJECT
public:
    Modbus_Plugin_Base();
    ~Modbus_Plugin_Base();

    bool check_break_flag() const;
    void clear_break_flag();

    const Config& base_config() const;

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

protected:
    void print_cached(QModbusClient* modbus, int server_address, QModbusDataUnit::RegisterType register_type, QModbusDevice::Error value, const QString& text);

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
    Controller* controller_by_device(Device* dev);
    Config dev_config(Device* dev, bool simple = false);

    Config base_config_;

    bool b_break;

    enum Auto_Port { NoAuto, UseFirstUSB, UseFirstTTY };
    int _auto_port = NoAuto;

    std::map<Thread*, std::vector<Device*>> _thread_map;
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_PLAGIN_BASE_H
