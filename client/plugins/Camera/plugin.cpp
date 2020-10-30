#include <QDebug>
#include <QSettings>
#include <QElapsedTimer>

#include <Helpz/settingshelper.h>

//#include <Das/scheme.h>
#include <Das/device_item.h>
//#include <Das/db/device_item_type.h>

#include "plugin.h"

namespace Das {

Camera_Plugin::Camera_Plugin() :
    QObject()
{}

Camera_Plugin::~Camera_Plugin()
{
    thread_.stop();
}

void Camera_Plugin::configure(QSettings *settings)
{
    using Helpz::Param;

    // device_item::index plugin param need to be set to one of these:
    // /dev/video0 - if it's USB camera
    // rtsp://10.10.2.141:554/user=admin_password=_channel=1_stream=0.sdp - if it's IP camera
    // For IP camera you can find correct URL on https://www.ispyconnect.com/man.aspx?n=china#

    Camera::Config config = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Param<QString>
        #endif
            (
                settings, "Camera",
                Param<QString>{"DevicePrefix", "/dev/video"},
                Param<QString>{"StreamServer", "deviceaccess.ru"},
                Param<QString>{"StreamServerPort", "6731"},
                Param<QString>{"BaseParamName", "cam"},
                Param<uint32_t>{"FrameDelayMs", 60},
                Param<uint32_t>{"PictureSkip", 50},
                Param<uint32_t>{"StreamWidth", 320},
                Param<uint32_t>{"StreamHeight", 240},
                Param<int32_t>{"Quality", -1}
    ).obj<Camera::Config>();

    thread_.start(std::move(config), this);
}

bool Camera_Plugin::check(Device* dev)
{
    if (!dev)
        return false;

    std::map<Device_Item*, Device::Data_Item> device_items_values;
    std::vector<Device_Item*> device_items_disconected;
    bool is_local;

    const Device::Data_Item data{/*user_id_=*/0, DB::Log_Base_Item::current_timestamp(), /*raw_data_=*/0};

    for (Device_Item* item: dev->items())
    {
        try
        {
            (void)thread_.get_device_path(item, is_local, /*skip_connected=*/true);

            if (!item->is_connected())
                device_items_values.emplace(item, data);
        }
        catch (...)
        {
            if (item->is_connected())
                device_items_disconected.push_back(item);
        }
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, Device::Data_Item>>
                                  ("std::map<Device_Item*, Device::Data_Item>", device_items_values),
                                  Q_ARG(bool, true));
    }

    if (!device_items_disconected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconected));
    }

    return true;
}

void Camera_Plugin::stop()
{
    thread_.stop();
}

void Camera_Plugin::write(std::vector<Write_Cache_Item>& /*items*/) {}

void Camera_Plugin::toggle_stream(uint32_t user_id, Das::Device_Item *item, bool state)
{
    qCDebug(CameraLog).nospace() << user_id << "|Toggle stream " << item->display_name() << " to: " << state;
    thread_.toggle_stream(user_id, item, state);
}

void Camera_Plugin::save_frame(Device_Item *item)
{
    thread_.save_frame(item);
}

} // namespace Das
