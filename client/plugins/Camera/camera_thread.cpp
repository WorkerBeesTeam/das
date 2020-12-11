#include <future>

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

Q_LOGGING_CATEGORY(CameraLog, "cam")

void Camera_Thread::start(Checker::Interface *iface)
{
    _iface = iface;
    _cam_param_type = iface->scheme()->param_mng_.get_type(config()._base_param_name);

    _break = false;

    thread th(&Camera_Thread::run, this);
    _thread.swap(th);
}

void Camera_Thread::stop()
{
    {
        lock_guard lock(_mutex);
        _break = true;
    }
    _cond.notify_one();

    if (_thread.joinable())
        _thread.join();
}

future<QByteArray> Camera_Thread::start_stream(uint32_t user_id, Device_Item *item, const QString &url)
{
    auto data = make_shared<Stream_Data>(Data::T_STREAM_START, item, user_id, url);
    future<QByteArray> res = data->_promise.get_future();

    {
        lock_guard lock(_mutex);
        _data.push(move(data));
    }
    _cond.notify_one();
    return res;
}

void Camera_Thread::stop_stream(const QString &url)
{
    {
        auto data = make_shared<Stop_Stream_Data>(Data::T_STREAM_STOP, url);
        lock_guard lock(_mutex);
        _data.push(move(data));
    }
    _cond.notify_one();
}

void Camera_Thread::save_frame(Device_Item *item)
{
    {
        auto data = make_shared<Dev_Item_Data>(Data::T_TAKE_PICTURE, item);
        lock_guard lock(_mutex);
        _data.push(move(data));
    }
    _cond.notify_one();
}

string Camera_Thread::get_device_path(Device_Item *item, bool& is_local_cam, bool skip_connected) const
{
    if (!skip_connected && !item->is_connected())
        throw runtime_error("Device item is not connected");

    if (item->register_type() != Device_Item_Type::RT_VIDEO_STREAM)
        throw runtime_error("Device item is not supported type");

    const QString index_param = item->param("index").toString();
    if (index_param.isEmpty())
        throw runtime_error("Device item hasn't param");

    Param* cam_param = item->group()->params()->get_by_type_id(_cam_param_type->id());
    if (!cam_param)
        throw runtime_error("Base camera param not found");

    cam_param = cam_param->get('n' + index_param);
    if (!cam_param)
        throw runtime_error("Camera param not found");

    QString path = cam_param->value().toString();
    if (path.isEmpty())
    {
        path = config()._device_prefix + index_param;
        QMetaObject::invokeMethod(cam_param, "set_value", Qt::QueuedConnection, Q_ARG(QVariant, path));
    }

    QUrl url(path);
    if (url.isEmpty() || !url.isValid())
        throw runtime_error("Unknown camera url");

    is_local_cam = url.scheme().isEmpty();

    if (is_local_cam && !QFile::exists(path))
        throw runtime_error("Device not found");

    return path.toStdString();
}

const Camera::Config &Camera_Thread::config() const
{
    return Camera::Config::instance();
}

void Camera_Thread::run()
{
    unique_lock lock(_mutex, defer_lock);
    auto check_func = [this]()
    {
        return _break
                || !_data.empty()
                || !_streams.empty()
                || !_sockets.empty()
                || !_stream_to_socket.empty();
    };

    while(true)
    {
        lock.lock();
        if (_wait_timeout.has_value())
            _cond.wait_for(lock, *_wait_timeout, check_func);
        else
            _cond.wait(lock, check_func);

        if (_break)
            break;

        if (!_data.empty())
        {
            shared_ptr<Data> data = move(_data.front());
            _data.pop();
            lock.unlock();

            switch (data->_type)
            {
            case Data::T_TAKE_PICTURE: read_item(static_pointer_cast<Dev_Item_Data>(data)->_item); break;
            case Data::T_STREAM_START: start_stream(static_pointer_cast<Stream_Data>(data)); break;
            case Data::T_STREAM_STOP: stop_stream(static_pointer_cast<Stop_Stream_Data>(data)); break;

            default:
                break;
            }
        }
        else
        {
            lock.unlock();
            process_streams();
        }
    }

    _stream_to_socket.clear();
    _streams.clear();
    _sockets.clear();
}

void Camera_Thread::start_stream(shared_ptr<Stream_Data> data)
{
    try
    {
        QUrl url{data->_url};
        if (!url.isValid() || url.isEmpty())
            throw runtime_error("Bad url");

        auto stream_it = _streams.find(data->_item);
        if (stream_it == _streams.end())
        {
            shared_ptr<Camera_Stream_Iface> stream = open_stream(data->_item,
                                                                  config()._stream_width,
                                                                  config()._stream_height);
            auto it = _streams.emplace(data->_item, move(stream));
            if (!it.second)
                throw runtime_error("Failed create stream");
            stream_it = it.first;
        }

        const Camera_Stream_Iface& stream = *stream_it->second;
        data->_promise.set_value(stream.param());

        auto socket_it = _sockets.find(data->_url);
        if (stream_it == _streams.end())
        {
            const QString url_str = data->_url;
            shared_ptr<Stream_Client_Thread> socket = make_shared<Stream_Client_Thread>(
                        url.host().toStdString(), to_string(url.port(config()._default_stream_port)),
                        [this, url_str](){ stop_stream(url_str); });
            auto it = _sockets.emplace(data->_url, move(socket));
            if (!it.second)
            {
                _streams.erase(stream_it);
                throw runtime_error("Failed open socket");
            }
            socket_it = it.first;
        }

        _stream_to_socket[data->_item].insert(socket_it->second);
        qCDebug(CameraLog).nospace() << data->_user_id << "|Start stream " << data->_item->display_name() << ' '
                                     << stream.width() << 'x' << stream.height();
    }
    catch (...)
    {
        data->_promise.set_exception(current_exception());
    }
}

void Camera_Thread::stop_stream(shared_ptr<Stop_Stream_Data> data)
{
    auto socket_it = _sockets.find(data->_url);
    if (socket_it != _sockets.end())
    {
        for (auto it = _stream_to_socket.begin(); it != _stream_to_socket.end(); ++it)
            it->second.erase(socket_it->second);
        _sockets.erase(socket_it);
    }
}

void Camera_Thread::process_streams()
{
    uint16_t count = 0;
    for (auto it = _stream_to_socket.begin(); it != _stream_to_socket.end(); )
    {
        auto stream_it = _streams.find(it->first);
        if (stream_it != _streams.end())
        {
            Send_Status status = send_stream_data(it->first, stream_it->second.get(), it->second);
            if (status == S_OK)
                ++count;

            if (status == S_FAIL)
            {
                set<shared_ptr<Stream_Client_Thread>> sockets{std::move(it->second)};
                it = _stream_to_socket.erase(it);
                _streams.erase(stream_it);
                remove_sockets(sockets);
            }
            else
                ++it;
        }
        else
            it = _stream_to_socket.erase(it);
    }

    if (count)
        _wait_timeout = chrono::milliseconds(config()._frame_delay);
    else if (_stream_to_socket.empty())
    {
        _streams.clear();
        _sockets.clear();
    }
}

shared_ptr<Camera_Stream_Iface> Camera_Thread::open_stream(Device_Item *item, uint32_t width, uint32_t height)
{
    bool is_local_cam;
    const string path = get_device_path(item, is_local_cam);

    if (is_local_cam)
        return make_shared<Camera_Stream>(path, width, height, config()._quality);
    else
        return make_shared<RTSP_Stream>(path, width, height, config()._rtsp_skip_frame_ms);
}

Camera_Thread::Send_Status Camera_Thread::send_stream_data(Device_Item *item, Camera_Stream_Iface *stream,
                                                           const set<shared_ptr<Stream_Client_Thread>>& sockets)
{
    auto res = S_WAITING_PREV;
    try
    {
        const QByteArray buffer = stream->get_frame();

        if (sockets.empty())
            throw runtime_error("No socket");

        for (auto&& socket: sockets)
        {
            if (socket->is_frame_sended(item->id()))
            {
                socket->send(item->id(), stream->param(), buffer, config()._frame_send_timeout_ms);
                res = S_OK;
            }
        }
    }
    catch (const exception& e)
    {
        qCCritical(CameraLog) << "get_frame failed:" << e.what();
        return S_FAIL;
    }
    return res;
}

void Camera_Thread::read_item(Device_Item *item)
{
    QByteArray data = "img:";

    auto it = _streams.find(item);

    try
    {
        for (const pair<Device_Item*, set<shared_ptr<Stream_Client_Thread>>>& it: _stream_to_socket)
        {
            auto stream_it = _streams.find(it.first);
            if (stream_it != _streams.cend())
                for (const shared_ptr<Stream_Client_Thread>& socket: it.second)
                    socket->send_text(it.first->id(), stream_it->second->param(), it.first == item ? "Сохранение..." : "...");
        }

        if (it != _streams.end())
        {
            shared_ptr<Camera_Stream_Iface>& stream = it->second;

            uint32_t width = stream->width();
            uint32_t height = stream->height();

            stream->reinit();
            stream->set_skip_frame_count(config()._picture_skip);
            data += stream->get_frame().toBase64();
            stream->reinit(width, height);
        }
        else
        {
            shared_ptr<Camera_Stream_Iface> stream = open_stream(item, 0, 0);
            stream->set_skip_frame_count(config()._picture_skip);
            data += stream->get_frame().toBase64();
        }
    }
    catch (const exception& e)
    {
        qCCritical(CameraLog) << "Save frame failed:" << e.what() << "item:" << item->display_name();
        return;
    }

    const qint64 now = Log_Value_Item::current_timestamp();
    const Log_Value_Item log_value_item{now, /*user_id=*/0, item->id(), data, QVariant(), /*need_to_save=*/true};
    emit _iface->scheme()->log_item_available(log_value_item);
    QMetaObject::invokeMethod(item, "set_raw_value", Qt::QueuedConnection, Q_ARG(QVariant, 0), /*force=*/Q_ARG(bool, true));
}

void Camera_Thread::remove_sockets(const set<shared_ptr<Stream_Client_Thread>>& sockets)
{
    for (const shared_ptr<Stream_Client_Thread>& socket: sockets)
    {
        auto sts_it = find_if(_stream_to_socket.cbegin(), _stream_to_socket.cend(),
                              [&socket](const pair<Device_Item*, set<shared_ptr<Stream_Client_Thread>>>& item)
        {
            return item.second.find(socket) != item.second.cend();
        });
        if (sts_it == _stream_to_socket.cend())
            for (auto it = _sockets.begin(); it != _sockets.end(); ++it)
                if (it->second == socket)
                {
                    _sockets.erase(it);
                    break;
                }
    }
}

} // namespace Das
