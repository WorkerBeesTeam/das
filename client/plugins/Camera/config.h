#ifndef DAS_CAMERA_PLUGIN_CONFIG_H
#define DAS_CAMERA_PLUGIN_CONFIG_H

//#include <chrono>
#include <QString>

namespace Das {
namespace Camera {

struct Config {
    Config& operator=(const Config&) = default;
    Config(const Config&) = default;
    Config(Config&&) = default;
    Config() = default;

    static Config& instance();

    QString _device_prefix = "/dev/video";
    std::string _stream_server = "deviceaccess.ru";
    std::string _stream_server_port = "6731";
    QString _base_param_name = "cam";
    uint32_t _frame_delay = 60;
    uint32_t _frame_send_timeout_ms = 1500;
    uint32_t _picture_skip = 50;
    uint32_t _stream_width = 320;
    uint32_t _stream_height = 240;
    int32_t _quality = -1;
};

} // namespace Camera
} // namespace Das

#endif // DAS_CAMERA_PLUGIN_CONFIG_H
