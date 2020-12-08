#ifndef DAS_CAMERA_THREAD_H
#define DAS_CAMERA_THREAD_H

#include <queue>

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

class Camera_Thread
{
public:
    void start(Checker::Interface* iface);
    void stop();

    void toggle_stream(uint32_t user_id, Device_Item *item, bool state);

    void save_frame(Device_Item* item);

    std::string get_device_path(Device_Item* item, bool& is_local_cam, bool skip_connected = false) const;
private:
    const Camera::Config& config() const;

    void run();

    enum Send_Status
    {
        S_FAIL,
        S_OK,
        S_WAITING_PREV
    };

    std::shared_ptr<Camera_Stream_Iface> open_stream(Device_Item* item, uint32_t width = 0, uint32_t height = 0);
    Send_Status send_stream_data(Device_Item *item, Camera_Stream_Iface* stream);
    void read_item(Device_Item *item);

    bool break_;
    Checker::Interface* iface_;
    DIG_Param_Type *_cam_param_type;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    enum Data_Type {
        DT_UNKNOWN,
        DT_TAKE_PICTURE,
        DT_STREAM_START,
        DT_STREAM_STOP,
    };

    struct Data {
        uint32_t user_id_;
        Device_Item* item_;
        Data_Type type_;
    };

    std::queue<Data> read_queue_;

    std::map<Device_Item*, std::shared_ptr<Camera_Stream_Iface>> streams_;

    std::unique_ptr<Stream_Client_Thread> socket_;
};

} // namespace Das

#endif // DAS_CAMERA_THREAD_H
