#include <QFile>
#include <QUrl>

#include <Das/db/device_item_type.h>
#include <Das/param/paramgroup.h>
#include <Das/device_item_group.h>
#include <Das/device_item.h>
#include <Das/scheme.h>

#include "camera_stream.h"
#include "rtsp_stream.h"
#include "camera_thread.h"

namespace Das {

Q_LOGGING_CATEGORY(CameraLog, "camera")

void Camera_Thread::start(Checker::Interface *iface)
{
    iface_ = iface;
    _cam_param_type = iface->scheme()->param_mng_.get_type(config()._base_param_name);

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

std::string Camera_Thread::get_device_path(Device_Item *item, bool& is_local_cam, bool skip_connected) const
{
    if (!skip_connected && !item->is_connected())
        throw std::runtime_error("Device item is not connected");

    if (item->register_type() != Device_Item_Type::RT_VIDEO_STREAM)
        throw std::runtime_error("Device item is not supported type");

    const QString index_param = item->param("index").toString();
    if (index_param.isEmpty())
        throw std::runtime_error("Device item hasn't param");

    Param* cam_param = item->group()->params()->get_by_type_id(_cam_param_type->id());
    if (!cam_param)
        throw std::runtime_error("Base camera param not found");

    cam_param = cam_param->get('n' + index_param);
    if (!cam_param)
        throw std::runtime_error("Camera param not found");

    QString path = cam_param->value().toString();
    if (path.isEmpty())
    {
        path = config()._device_prefix + index_param;
        QMetaObject::invokeMethod(cam_param, "set_value", Qt::QueuedConnection, Q_ARG(QVariant, path));
    }

    QUrl url(path);
    if (url.isEmpty() || !url.isValid())
        throw std::runtime_error("Unknown camera url");

    is_local_cam = url.scheme().isEmpty();

    if (is_local_cam && !QFile::exists(path))
        throw std::runtime_error("Device not found");

    return path.toStdString();
}

const Camera::Config &Camera_Thread::config() const
{
    return Camera::Config::instance();
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
                    if (streams_.find(data.item_) != streams_.end())
                        throw std::runtime_error("Already started");

                    std::shared_ptr<Camera_Stream_Iface> stream = open_stream(data.item_,
                                                                              config()._stream_width,
                                                                              config()._stream_height);

                    const QByteArray param = stream->param();

                    qCDebug(CameraLog).nospace() << data.user_id_ << "|Start stream " << stream->width() << 'x' << stream->height();

                    auto it = streams_.emplace(data.item_, std::move(stream));
                    if (!it.second)
                        throw std::runtime_error("Emplace failed");

                    if (!socket_)
                        socket_.reset(new Stream_Client_Thread(config()._stream_server,
                                                               config()._stream_server_port));

                    iface_->manager()->send_stream_param(data.item_, param);
                    iface_->manager()->send_stream_toggled(data.user_id_, data.item_, true);
                }
                catch (const std::exception& e)
                {
                    qCCritical(CameraLog).nospace() << data.user_id_ << "|Start stream " << data.item_->display_name() << " failed: " << e.what();
                    iface_->manager()->send_stream_toggled(data.user_id_, data.item_, false);
                    socket_.reset();
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

            uint16_t count = 0;
            for (auto it = streams_.begin(); it != streams_.end(); )
            {
                Send_Status status = send_stream_data(it->first, it->second.get());
                if (status == S_OK)
                    ++count;

                if (status == S_FAIL)
                {
                    iface_->manager()->send_stream_toggled(/*user_id=*/0, it->first, /*state=*/false);
                    it = streams_.erase(it);
                }
                else
                    ++it;
            }

//            if (count)
//                std::this_thread::sleep_for(std::chrono::milliseconds(config()._frame_delay));
        }
    }
}

std::shared_ptr<Camera_Stream_Iface> Camera_Thread::open_stream(Device_Item *item, uint32_t width, uint32_t height)
{
    bool is_local_cam;
    const std::string path = get_device_path(item, is_local_cam);

    if (is_local_cam)
        return std::make_shared<Camera_Stream>(path, width, height, config()._quality);
    else
        return std::make_shared<RTSP_Stream>(path, width, height, config()._rtsp_skip_frame_ms);
}

Camera_Thread::Send_Status Camera_Thread::send_stream_data(Device_Item *item, Camera_Stream_Iface *stream)
{
    try
    {
        if (!socket_)
            throw std::runtime_error("No socket");

        const QByteArray buffer = stream->get_frame();

        if (!socket_->is_frame_sended(item->id()))
            return S_WAITING_PREV;

        socket_->send(item->id(), stream->param(), buffer, config()._frame_send_timeout_ms);
//        iface_->manager()->send_stream_data(item, stream->get_frame());
    }
    catch (const std::exception& e)
    {
        socket_.reset();
        qCCritical(CameraLog) << "get_frame failed:" << e.what();
        return S_FAIL;
    }
    return S_OK;
}

void Camera_Thread::read_item(Device_Item *item)
{
    QByteArray data = "img:";

    auto it = streams_.find(item);

    try
    {
        if (socket_)
        {
            for (const std::pair<Device_Item*, std::shared_ptr<Camera_Stream_Iface>>& stream: streams_)
                socket_->send_text(stream.first->id(), stream.second->param(), stream.first == item ? "Сохранение..." : "...");
        }

        if (it != streams_.end())
        {
            std::shared_ptr<Camera_Stream_Iface>& stream = it->second;

            uint32_t width = stream->width();
            uint32_t height = stream->height();

            stream->reinit();
            stream->set_skip_frame_count(config()._picture_skip);
            data += stream->get_frame().toBase64();
            stream->reinit(width, height);
        }
        else
        {
            std::shared_ptr<Camera_Stream_Iface> stream = open_stream(item, 0, 0);
            stream->set_skip_frame_count(config()._picture_skip);
            data += stream->get_frame().toBase64();
        }
    }
    catch (const std::exception& e)
    {
        qCCritical(CameraLog) << "Save frame failed:" << e.what() << "item:" << item->display_name();
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

} // namespace Das
