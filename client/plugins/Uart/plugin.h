#ifndef DAS_UART_PLUGIN_H
#define DAS_UART_PLUGIN_H

#include <memory>
#include <set>
#include <queue>
#include <functional>

//#include <thread>
#include <mutex>
#include <condition_variable>

#include <QThread>
#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checkerinterface.h>
#include <Das/device.h>

#include "config.h"

namespace Das {
Q_DECLARE_LOGGING_CATEGORY(UartLog)

class Uart_Thread : public QThread
{
public:
    bool joinable() const { return isRunning(); }
    bool join() { return wait(); }

    void start(Uart::Config config);
    bool check(Device *dev);
    void stop();
    void write(std::vector<Write_Cache_Item>& items);
private:
    void run();
    void set_config(QSerialPort& port);
    void reconnect(QSerialPort& port);

    void read_item(QSerialPort& port, Device_Item* item);
    void write_item(QSerialPort& port, const Write_Cache_Item& item);

    bool break_, is_port_name_in_config_;
    Uart::Config config_;

//    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    std::set<Device_Item*> read_set_, write_set_;
    std::queue<Device_Item*> read_queue_;
    std::queue<Write_Cache_Item> write_queue_;

    std::map<Device*, std::map<Device_Item*, Device::Data_Item>> device_items_values_;
    std::map<Device*, std::vector<Device_Item*>> device_items_disconected_;
};

class DAS_PLUGIN_SHARED_EXPORT Uart_Plugin : public QObject, public Checker_Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker_Interface)
public:
    Uart_Plugin();
    ~Uart_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings, Scheme* scheme) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
private:
    Uart_Thread thread_;
};

} // namespace Das

#endif // DAS_UART_PLUGIN_H
