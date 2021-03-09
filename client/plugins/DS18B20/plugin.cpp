#include <cmath>
#include <exception>

#include <Helpz/settingshelper.h>

#include <Das/device.h>

#ifndef NO_WIRINGPI
#include <wiringPi.h>
#include "one_wire.h"
#endif

#include "plugin.h"

namespace Das {

Q_LOGGING_CATEGORY(DS18B20Log, "DS18B20")

// ----------

DS18B20_Plugin::DS18B20_Plugin() :
    QObject(),
    _is_error_printed(false),
    _rom_count(0),
    _one_wire(nullptr)
{
}

DS18B20_Plugin::~DS18B20_Plugin()
{
    stop();
    if (_thread.joinable())
        _thread.join();
}

void DS18B20_Plugin::configure(QSettings *settings)
{
    using Helpz::Param;
    auto [pin] = Helpz::SettingsHelper<Param<uint16_t>>
                                                        (settings, "DS18B20",
                                                           Param<uint16_t>{"Pin", 7}
                                                           )();
    _thread = std::thread{&DS18B20_Plugin::run, this, pin};
}

bool DS18B20_Plugin::check(Device* dev)
{
    std::vector<Data_Item> rom_item_vect;
    QVariant raw_num;
    uint32_t num;
    bool is_ok;

    for (Device_Item * item: dev->items())
    {
        raw_num = item->param("num");
        if (raw_num.isValid())
        {
            num = raw_num.toUInt(&is_ok);
            if (is_ok)
                rom_item_vect.push_back(Data_Item{num, item});
        }
    }

    if (!rom_item_vect.empty())
        add_roms_to_queue(std::move(rom_item_vect));
    return true;
}

void DS18B20_Plugin::stop()
{
    _break = true;
    _cond.notify_one();
}

void DS18B20_Plugin::write(Device */*dev*/, std::vector<Write_Cache_Item>& /*items*/) {}

void DS18B20_Plugin::add_roms_to_queue(std::vector<DS18B20_Plugin::Data_Item> rom_item_vect)
{
    {
        std::lock_guard lock(_mutex);

        for (auto data_it = rom_item_vect.begin(); data_it != rom_item_vect.end(); ++data_it)
        {
            Data_Item& data = *data_it;
            const auto it = std::find(_data.cbegin(), _data.cend(), data._rom);
            if (it == _data.cend())
                _data.push_back(std::move(data));
        }
    }

    _cond.notify_one();
}

void DS18B20_Plugin::run(uint16_t pin)
{
    if (!init(pin))
        return;

    std::unique_lock lock(_mutex, std::defer_lock);

    while (!_break)
    {
        lock.lock();
        _cond.wait(lock, [this]() { return _break || !_data.empty(); });

        if (_break)
            break;

        const Data_Item data = _data.front();
        lock.unlock();

        process_item(data._rom, data._item);

        lock.lock();
        if (!_data.empty() && data._rom == _data.front()._rom)
            _data.pop_front();
        lock.unlock();
    }

    delete _one_wire;
}

bool DS18B20_Plugin::init(uint16_t pin)
{
#ifndef NO_WIRINGPI
    try
    {
        _one_wire = new One_Wire(pin);
        _one_wire->init();
        search_rom();
        return true;
    }
    catch(const std::exception& e)
    {
        qCCritical(DS18B20Log, "%s", e.what());
    }

    if (_one_wire)
        delete _one_wire;
#endif
    return false;
}

void DS18B20_Plugin::search_rom()
{
    int n = 100;
    uint64_t roms[n];
#ifndef NO_WIRINGPI
    QString error_text;
    try
    {
        _one_wire->search_rom(roms, n, _break);
    }
    catch(const std::exception& e)
    {
        n = 0;
        error_text = QString::fromLocal8Bit(e.what());
    }
#else
    n = 0;
#endif

    if (n)
    {
        _rom_count = n;
        _rom_array.reset(new uint64_t[n]);
        memcpy(_rom_array.get(), roms, n * sizeof(uint64_t));

        if (_is_error_printed)
            _is_error_printed = false;
    }
    else
    {
        if (_rom_array)
            _rom_array.reset();

        if (!_is_error_printed)
        {
            _is_error_printed = true;
            qCCritical(DS18B20Log) << "Unable to find temperature sensor:";
        }
    }
}

void DS18B20_Plugin::process_item(uint32_t rom, Device_Item *item)
{
    if (!_rom_array || _rom_count < rom)
        search_rom();

    bool is_ok;
    double value = get_temperature(rom, is_ok);
    if (is_ok)
    {
        value = std::floor(value * 10) / 10;

        std::map<Device_Item*, Device::Data_Item> data;
        data.emplace(item, Device::Data_Item{0, DB::Log_Base_Item::current_timestamp(), value});
        QMetaObject::invokeMethod(item->device(), "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*,Device::Data_Item>>{"std::map<Device_Item*,Device::Data_Item>", data},
                                  Q_ARG(bool, true));
    }
    else
        QMetaObject::invokeMethod(item->device(), "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, {item}));
}

double DS18B20_Plugin::get_temperature(uint32_t num, bool &is_ok)
{
#ifndef NO_WIRINGPI
    if (num < _rom_count)
    {
        uint64_t rom = _rom_array[num];

        uint8_t data[9], attempt = 0;

        do
        {
            _one_wire->set_device(rom);
            _one_wire->write_byte(CMD_CONVERTTEMP);

            delay(750);

            _one_wire->set_device(rom);
            _one_wire->write_byte(CMD_RSCRATCHPAD);

            for (int i = 0; i < 9; i++)
                data[i] = _one_wire->read_byte();
        }
        while (!_break && _one_wire->crc8(data, 8) != data[8] && ++attempt < 5);

        if (!_break && attempt < 5)
        {
            is_ok = true;
            return static_cast<int16_t>(((data[1] << 8) | data[0])) * 0.0625;
        }
    }

    is_ok = false;
    return 0;
#else
    is_ok = true;
    return num + 36.6;
#endif
}

} // namespace Das
