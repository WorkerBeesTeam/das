#ifndef DAS_CAMERA_PLUGIN_H
#define DAS_CAMERA_PLUGIN_H

#include <memory>
#include <set>
#include <queue>
#include <functional>

//#include <thread>
#include <mutex>
#include <condition_variable>

#include <QThread>
#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checker_interface.h>
#include <Das/device.h>

#include "video_stream.h"
#include "config.h"

namespace Das {
Q_DECLARE_LOGGING_CATEGORY(CameraLog)

class Stream_Client_Thread;
class Camera_Thread
{
public:
    void start(Camera::Config config, Checker::Interface* iface);
    void stop();

    void toggle_stream(uint32_t user_id, Device_Item *item, bool state);

    void save_frame(Device_Item* item);

    QString get_device_path(Device_Item* item, bool skip_connected = false) const;
    const Camera::Config& config() const;
private:
    void run();

    bool send_stream_data(Device_Item *item, Video_Stream* stream);
    void read_item(Device_Item *item);

    bool break_;
    Checker::Interface* iface_;
    Camera::Config config_;

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

    std::map<Device_Item*, std::unique_ptr<Video_Stream>> streams_;

    std::unique_ptr<Stream_Client_Thread> socket_;
};

class DAS_PLUGIN_SHARED_EXPORT Camera_Plugin : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)
public:
    Camera_Plugin();
    ~Camera_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
    void toggle_stream(uint32_t user_id, Device_Item *item, bool state) override;
public slots:
    void save_frame(Device_Item* item);
private:
    Camera_Thread thread_;
};

} // namespace Das

#endif // DAS_UART_PLUGIN_H
