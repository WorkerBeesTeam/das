#ifndef DAS_HTU21PLUGIN_H
#define DAS_HTU21PLUGIN_H

#include <memory>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checker_interface.h>

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(DS18B20Log)

class One_Wire;
class DAS_PLUGIN_SHARED_EXPORT DS18B20_Plugin : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)

public:
    DS18B20_Plugin();
    ~DS18B20_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override final;
    void write(std::vector<Write_Cache_Item>& items) override;
private:

    struct Data_Item
    {
        uint32_t _rom;
        Device_Item* _item;

        bool operator ==(uint32_t rom) const
        {
            return _rom == rom;
        }
    };

    void add_roms_to_queue(std::vector<Data_Item> rom_item_vect);

    void run(uint16_t pin);
    bool init(uint16_t pin);
    void search_rom();
    void process_item(uint32_t rom, Device_Item* item);
    double get_temperature(uint32_t num, bool& is_ok);

    bool _is_error_printed, _break = false;

    std::unique_ptr<uint64_t[]> _rom_array;
    uint32_t _rom_count;

    One_Wire* _one_wire;

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cond;

    std::deque<Data_Item> _data;
};

} // namespace Das

#endif // DAS_HTU21PLUGIN_H
