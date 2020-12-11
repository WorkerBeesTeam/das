#ifndef DAS_CAMERA_THREAD_H
#define DAS_CAMERA_THREAD_H

#include <optional>
#include <queue>
#include <set>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <QLoggingCategory>

#include <Das/checker_interface.h>
#include <Das/db/dig_param_type.h>

#include "config.h"
#include "camera_stream_iface.h"
#include "stream/stream_client_thread.h"

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(CameraLog)

struct Data
{
    enum Type {
        T_UNKNOWN,
        T_TAKE_PICTURE,
        T_STREAM_START,
        T_STREAM_STOP
    };

    Data(Type type) : _type{type} {}
    Type _type;
};

struct Dev_Item_Data : Data
{
    Dev_Item_Data(Type type, Device_Item* item) : Data{type}, _item{item} {}
    Device_Item* _item;
};

struct Stream_Data : Dev_Item_Data
{
    Stream_Data(Type type, Device_Item* item, uint32_t user_id, const QString& url) :
        Dev_Item_Data{type, item}, _user_id{user_id}, _url{url} {}
    uint32_t _user_id;
    promise<QByteArray> _promise;
    QString _url; // udp://deviceaccess.ru:6731
};

struct Stop_Stream_Data : Data
{
    Stop_Stream_Data(Type type, const QString& url) : Data{type}, _url{url} {}
    QString _url;
};

class Camera_Thread
{
public:
    void start(Checker::Interface* iface);
    void stop();

    future<QByteArray> start_stream(uint32_t user_id, Device_Item *item, const QString &url);
    void stop_stream(const QString& url);

    void save_frame(Device_Item* item);

    string get_device_path(Device_Item* item, bool& is_local_cam, bool skip_connected = false) const;
private:
    const Camera::Config& config() const;

    void run();

    enum Send_Status
    {
        S_FAIL,
        S_OK,
        S_WAITING_PREV
    };

    void start_stream(shared_ptr<Stream_Data> data);
    void stop_stream(shared_ptr<Stop_Stream_Data> data);
    void process_streams();
    shared_ptr<Camera_Stream_Iface> open_stream(Device_Item* item, uint32_t width = 0, uint32_t height = 0);
    Send_Status send_stream_data(Device_Item *item, Camera_Stream_Iface* stream, const set<shared_ptr<Stream_Client_Thread>> &sockets);
    void read_item(Device_Item *item);
    void remove_sockets(const set<shared_ptr<Stream_Client_Thread>>& sockets);

    bool _break;
    Checker::Interface* _iface;
    DIG_Param_Type *_cam_param_type;

    optional<chrono::milliseconds> _wait_timeout;

    thread _thread;
    mutex _mutex;
    condition_variable _cond;

    queue<shared_ptr<Data>> _data;

    map<Device_Item*, shared_ptr<Camera_Stream_Iface>> _streams;
    map<QString, shared_ptr<Stream_Client_Thread>> _sockets;
    map<Device_Item*, set<shared_ptr<Stream_Client_Thread>>> _stream_to_socket;
};

} // namespace Das

#endif // DAS_CAMERA_THREAD_H
