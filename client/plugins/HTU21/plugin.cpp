#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <QDebug>
#include <QSettings>
#include <QFile>

#ifndef NO_WIRINGPI
#include <wiringPi.h>
#include <wiringPiI2C.h>
#endif

#include <Helpz/settingshelper.h>
#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/scheme.h>

#include "plugin.h"

#define   HTU21D_I2C_ADDR 0x40

#define   HTU21D_TEMP     0xF3
#define   HTU21D_HUMID    0xF5

namespace Das {

Q_LOGGING_CATEGORY(HTU21Log, "HTU21")

// ----------

HTU21Plugin::HTU21Plugin() :
    QObject(),
    is_error_printed_(false)
{
}

HTU21Plugin::~HTU21Plugin()
{
}

void HTU21Plugin::configure(QSettings *settings, Checker::Manager_Interface */*mng*/, Scheme *scheme)
{
    /*
    using Helpz::Param;
    conf = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Param<QString>,Param<QSerialPort::BaudRate>,Param<QSerialPort::DataBits>,
                            Param<QSerialPort::Parity>,Param<QSerialPort::StopBits>,Param<QSerialPort::FlowControl>,Param<int>,Param<int>,Param<int>>
        #endif
            (
                settings, "HTU21",
                Param<QString>{"Port", "ttyUSB0"},
    ).unique_ptr<Conf>();
    */

#ifndef NO_WIRINGPI
    wiringPiSetup();
#endif
}

bool HTU21Plugin::check(Device* dev)
{
#ifndef NO_WIRINGPI
    int fd = wiringPiI2CSetup(HTU21D_I2C_ADDR);
    if (fd < 0)
    {
        if (!is_error_printed_)
        {
            qCCritical(HTU21Log, "Unable to open I2C device: %s\n", strerror (errno));
            is_error_printed_ = true;
        }

        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, dev->items().toStdVector()));
        return false;
    }
    else if (is_error_printed_)
        is_error_printed_ = false;
#else
    int fd = 0;
#endif

    std::map<Device_Item*, Device::Data_Item> device_items_values;
    std::vector<Device_Item*> device_items_disconnected;
    QString type;
    bool is_error;
    double value;

    const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

    for (Device_Item * item: dev->items())
    {
        is_error = true;
        type = item->param("type").toString().trimmed().toLower();
        if (type == "temp")
            value = get_temperature(fd, is_error);
        else if (type == "humidity")
            value = get_humidity(fd, is_error);

        if (is_error)
            device_items_disconnected.push_back(item);
        else
        {
            Device::Data_Item data_item{0, timestamp_msecs, std::floor(value * 10) / 10};
            device_items_values.emplace(item, std::move(data_item));
        }
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

#ifndef NO_WIRINGPI
    close(fd);
#endif

    return true;
}

void HTU21Plugin::stop() {}
void HTU21Plugin::write(std::vector<Write_Cache_Item>& /*items*/) {}

double HTU21Plugin::get_sensor_value(int fd, int sensor_id, bool &is_error_out)
{
#ifndef NO_WIRINGPI
    if (wiringPiI2CWrite(fd, sensor_id) >= 0)
    {
        delay(100);
        unsigned char buf [4];
        if (read(fd, buf, 3) > 0)
        {
            is_error_out = false;
            unsigned int value = (buf [0] << 8 | buf [1]) & 0xFFFC;
            return value / 65536.0;
        }
    }

    is_error_out = true;
    return 0.;
#else
    is_error_out = false;
    return 0.5;
#endif
}

double HTU21Plugin::get_temperature(int fd, bool &is_error_out)
{
    // Convert sensor reading into temperature.
    // See page 14 of the datasheet
    return -46.85 + (175.72 * get_sensor_value(fd, HTU21D_TEMP, is_error_out));
}

double HTU21Plugin::get_humidity(int fd, bool &is_error_out)
{
    // Convert sensor reading into humidity.
    // See page 15 of the datasheet
    return -6.0 + (125.0 * get_sensor_value(fd, HTU21D_HUMID, is_error_out));
}

} // namespace Das
