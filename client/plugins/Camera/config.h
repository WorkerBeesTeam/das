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
};

} // namespace Camera
} // namespace Das

#endif // DAS_CAMERA_PLUGIN_CONFIG_H
