#include <QDebug>
#include <QSettings>
#include <QFile>
#include <QElapsedTimer>

#include <Helpz/settingshelper.h>

#include <Das/scheme.h>
#include <Das/device_item.h>
#include <Das/db/device_item_type.h>

#include "stream/stream_client_thread.h"
#include "plugin.h"

namespace Das {

Q_LOGGING_CATEGORY(CameraLog, "camera")

// ----------------------------------------------------------

void Camera_Thread::start(Camera::Config config, Checker::Interface *iface)
{
    iface_ = iface;
    config_ = std::move(config);

    break_ = false;

    std::thread th(&Camera_Thread::run, this);
    thread_.swap(th);
}

void Camera_Thread::stop()
{
    {
        std::lock_guard lock(mutex_);
        break_ = true;
        cond_.notify_one();
    }

    if (thread_.joinable())
    {
        thread_.join();
    }
}

void Camera_Thread::toggle_stream(uint32_t user_id, Device_Item *item, bool state)
{
    Data data {user_id, item, state ? DT_STREAM_START : DT_STREAM_STOP};
    std::lock_guard lock(mutex_);
    read_queue_.push(std::move(data));
    cond_.notify_one();
}

void Camera_Thread::save_frame(Device_Item *item)
{
    Data data {0, item, DT_TAKE_PICTURE};
    std::lock_guard lock(mutex_);
    read_queue_.push(std::move(data));
    cond_.notify_one();
}

QString Camera_Thread::get_device_path(Device_Item *item, bool skip_connected) const
{
    if (!skip_connected && !item->is_connected())
        throw std::runtime_error("Device item is not connected");

    if (item->register_type() != Device_Item_Type::RT_VIDEO_STREAM)
        throw std::runtime_error("Device item is not supported type");

    const QString index_param = item->param("index").toString();
    if (index_param.isEmpty())
        throw std::runtime_error("Device item hasn't param");

    const QString path = config_.device_prefix_ + index_param;
    if (!QFile::exists(path))
        throw std::runtime_error("Device not found");

    return path;
}

const Camera::Config &Camera_Thread::config() const
{
    return config_;
}

void Camera_Thread::run()
{
    std::unique_lock lock(mutex_, std::defer_lock);
    while(true)
    {
        lock.lock();
        cond_.wait(lock, [this]()
        {
            return break_
                    || !read_queue_.empty()
                    || !streams_.empty();
        });

        if (break_)
        {
            // TODO: Clear streams
            break;
        }

        if (!read_queue_.empty())
        {
            Data data = std::move(read_queue_.front());
            read_queue_.pop();
            lock.unlock();

            switch (data.type_)
            {
            case DT_TAKE_PICTURE: read_item(data.item_); break;
            case DT_STREAM_START:
                try
                {
                    const QString path = get_device_path(data.item_);

                    if (streams_.find(data.item_) != streams_.end())
                        throw std::runtime_error("Already started");

                    if (!socket_)
                        socket_.reset(new Stream_Client_Thread(config().stream_server_.toStdString(), config().stream_server_port_.toStdString()));

                    std::unique_ptr<Video_Stream> stream(new Video_Stream(path, config().stream_width_, config().stream_height_, config().quality_));
                    const QByteArray param = stream->param();

                    qCDebug(CameraLog).nospace() << data.user_id_ << "|Start stream " << stream->width() << 'x' << stream->height();

                    auto it = streams_.emplace(data.item_, std::move(stream));
                    if (!it.second)
                        throw std::runtime_error("Emplace failed");

                    iface_->manager()->send_stream_param(data.item_, param);
                    iface_->manager()->send_stream_toggled(data.user_id_, data.item_, true);
                }
                catch (const std::exception& e)
                {
                    qCCritical(CameraLog).nospace() << data.user_id_ << "|Start stream " << data.item_->display_name() << " failed: " << e.what();
                    iface_->manager()->send_stream_toggled(data.user_id_, data.item_, false);
                }
                break;

            case DT_STREAM_STOP:
                streams_.erase(data.item_);
                iface_->manager()->send_stream_toggled(data.user_id_, data.item_, false);
                socket_.reset();
                break;

            default:
                break;
            }
        }
        else
        {
            lock.unlock();

            for (auto it = streams_.begin(); it != streams_.end(); )
            {
                if (!send_stream_data(it->first, it->second.get()))
                {
                    iface_->manager()->send_stream_toggled(/*user_id=*/0, it->first, /*state=*/false);
                    it = streams_.erase(it);
                }
                else
                    ++it;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(config().frame_delay_));
        }
    }
}

bool Camera_Thread::send_stream_data(Device_Item *item, Video_Stream *stream)
{
    try
    {
        if (!socket_)
            throw std::runtime_error("No socket");

        socket_->send(item->id(), stream->param(), stream->get_frame());
//        iface_->manager()->send_stream_data(item, stream->get_frame());
    }
    catch (const std::exception& e)
    {
        socket_.reset();
        qCCritical(CameraLog) << "get_frame failed:" << e.what();
        return false;
    }
    return true;
}

void Camera_Thread::read_item(Device_Item *item)
{
    QByteArray data = "img:";

    auto fill_data = [this, &data](std::unique_ptr<Video_Stream>& stream)
    {
        for (uint32_t i = 0; i < config().picture_skip_; ++i)
            stream->get_frame();
        data += stream->get_frame().toBase64();
    };

    auto it = streams_.find(item);

    try
    {
        if (it != streams_.end())
        {
            std::unique_ptr<Video_Stream>& stream = it->second;

            uint32_t width = stream->width();
            uint32_t height = stream->height();

            stream->reinit();
            fill_data(stream);
            stream->reinit(width, height);
        }
        else
        {
            const QString path = get_device_path(item);
            std::unique_ptr<Video_Stream> stream(new Video_Stream(path, 0, 0, config().quality_));

            fill_data(stream);
        }
    }
    catch (const std::exception& e)
    {
        qCCritical(CameraLog) << "Save frame failed:" << e.what() << ". item:" << item->display_name();
        if (it != streams_.end())
        {
            iface_->manager()->send_stream_toggled(0, it->first, false);
            streams_.erase(it);
            socket_.reset();
        }
        return;
    }

    const qint64 now = Log_Value_Item::current_timestamp();
    const Log_Value_Item log_value_item{now, /*user_id=*/0, item->id(), data, QVariant(), /*need_to_save=*/true};
    emit iface_->scheme()->log_item_available(log_value_item);
    QMetaObject::invokeMethod(item, "set_raw_value", Qt::QueuedConnection, Q_ARG(QVariant, 0), /*force=*/Q_ARG(bool, true));
}

// ----------------------------------------------------------

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

    Camera::Config config = Helpz::SettingsHelper
        #if (__cplusplus < 201402L) || (defined(__GNUC__) && (__GNUC__ < 7))
            <Param<QString>
        #endif
            (
                settings, "Camera",
                Param<QString>{"DevicePrefix", "/dev/video"},
                Param<QString>{"StreamServer", "deviceaccess.ru"},
                Param<QString>{"StreamServerPort", "6731"},
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

    const Device::Data_Item data{/*user_id_=*/0, DB::Log_Base_Item::current_timestamp(), /*raw_data_=*/0};

    for (Device_Item* item: dev->items())
    {
        try
        {
            const QString path = thread_.get_device_path(item, /*skip_connected=*/true);
            Q_UNUSED(path);

            if (!item->is_connected())
                device_items_values.emplace(item, data);
        }
        catch (...)
        {
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
