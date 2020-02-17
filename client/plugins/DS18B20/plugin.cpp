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
    is_error_printed_(false),
    rom_count_(0),
    one_wire_(nullptr)
{
}

DS18B20_Plugin::~DS18B20_Plugin()
{
    if (one_wire_)
        delete one_wire_;
}

void DS18B20_Plugin::configure(QSettings *settings, Scheme */*scheme*/)
{
    using Helpz::Param;
    auto [pin] = Helpz::SettingsHelper<Param<uint16_t>>
                                                        (settings, "DS18B20",
                                                           Param<uint16_t>{"Pin", 7}
                                                           )();

#ifndef NO_WIRINGPI
    one_wire_ = new One_Wire(pin);

    try
    {
        one_wire_->init();
        search_rom();
    }
    catch(const std::exception& e)
    {
        qCCritical(DS18B20Log, "%s", e.what());
    }
#endif
}

bool DS18B20_Plugin::check(Device* dev)
{
    std::map<uint32_t, Device_Item*> rom_item_vect_;
    QVariant raw_num;
    uint32_t num, max_num = 0;
    bool is_ok;

    for (Device_Item * item: dev->items())
    {
        raw_num = item->param("num");
        if (raw_num.isValid())
        {
            num = raw_num.toUInt(&is_ok);
            if (is_ok)
            {
                rom_item_vect_.emplace(num, item);
                if (max_num < num)
                    max_num = num;
            }
        }
    }

    if (rom_item_vect_.empty())
        return true;

    if (!rom_array_ || rom_count_ < max_num)
        search_rom();

    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    std::map<Device_Item*, Device::Data_Item> device_items_values;
    std::vector<Device_Item*> device_items_disconnected;
    double value;

    for (auto it = rom_item_vect_.begin(); it != rom_item_vect_.end(); ++it)
    {
        value = get_temperature(it->first, is_ok);
        if (is_ok)
        {
            Device::Data_Item data_item{0, timestamp_msecs, std::floor(value * 10) / 10};
            device_items_values.emplace(it->second, std::move(data_item));
        }
        else
            device_items_disconnected.push_back(it->second);
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>("std::map<Device_Item*, Device::Data_Item>", device_items_values),
                                  Q_ARG(bool, true));
    }

    if (!device_items_disconnected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconnected));
    }
    return true;
}

void DS18B20_Plugin::stop() {}
void DS18B20_Plugin::write(std::vector<Write_Cache_Item>& /*items*/) {}

void DS18B20_Plugin::search_rom()
{
    int n = 100;
    uint64_t roms[n];
#ifndef NO_WIRINGPI
    QString error_text;
    try
    {
        one_wire_->search_rom(roms, n);
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
        rom_count_ = n;
        rom_array_.reset(new uint64_t[n]);
        memcpy(rom_array_.get(), roms, n * sizeof(uint64_t));

        if (is_error_printed_)
            is_error_printed_ = false;
    }
    else
    {
        if (rom_array_)
            rom_array_.reset();

        if (!is_error_printed_)
        {
            qCCritical(DS18B20Log) << "Unable to find temperature sensor:";
            is_error_printed_ = true;
        }
    }
}

double DS18B20_Plugin::get_temperature(uint32_t num, bool &is_ok)
{
#ifndef NO_WIRINGPI
    if (num < rom_count_)
    {
        uint64_t rom = rom_array_[num];

        uint8_t data[9], attempt = 0;

        do
        {
            one_wire_->set_device(rom);
            one_wire_->write_byte(CMD_CONVERTTEMP);

            delay(750);

            one_wire_->set_device(rom);
            one_wire_->write_byte(CMD_RSCRATCHPAD);

            for (int i = 0; i < 9; i++)
                data[i] = one_wire_->read_byte();
        }
        while (one_wire_->crc8(data, 8) != data[8] && ++attempt < 5);

        if (attempt < 5)
        {
            is_ok = true;
            return ((data[1] << 8) + data[0]) * 0.0625;
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
