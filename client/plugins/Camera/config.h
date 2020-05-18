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

    QString device_prefix_;
    QString stream_server_;
    QString stream_server_port_;
    uint32_t frame_delay_;
    uint32_t picture_skip_;
    uint32_t stream_width_;
    uint32_t stream_height_;
    int32_t quality_;
};

} // namespace Camera
} // namespace Das

#endif // DAS_CAMERA_PLUGIN_CONFIG_H
