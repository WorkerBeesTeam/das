#ifndef DAS_UART_THREAD_H
#define DAS_UART_THREAD_H

#include <set>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <QThread>
#include <QLoggingCategory>

#include <Das/device.h>
#include <Das/write_cache_item.h>

#include "config.h"
#include "lua_engine.h"

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(UartLog)

class Uart_Thread : public QThread
{
public:
    ~Uart_Thread();

    bool joinable() const { return isRunning(); }
    bool join() { return wait(); }

    void start();
    bool check(Device *dev);
    void stop();
    void write(std::vector<Write_Cache_Item>& items);
private:
    void run();
    void set_config(QSerialPort& port);
    void reconnect(QSerialPort& port);

    void read_item(QSerialPort& port, Device_Item* item);
    Device::Data_Item read_item_impl(QSerialPort& port, Device_Item* item);
    void write_item(QSerialPort& port, const Write_Cache_Item& item);

    bool break_, is_port_name_in_config_, ok_open_;

//    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    std::set<Device_Item*> read_set_, write_set_;
    std::queue<Device_Item*> read_queue_;
    std::queue<Write_Cache_Item> write_queue_;

    std::map<Device*, std::map<Device_Item*, Device::Data_Item>> device_items_values_;
    std::map<Device*, std::vector<Device_Item*>> device_items_disconected_;

    Lua_Engine _lua;
};

} // namespace Das

#endif // DAS_UART_THREAD_H
