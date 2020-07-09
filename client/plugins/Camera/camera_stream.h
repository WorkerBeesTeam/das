#ifndef DAS_CAMERA_PLUGIN_CAMERA_STREAM_H
#define DAS_CAMERA_PLUGIN_CAMERA_STREAM_H

#include <vector>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct v4lconvert_data;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <QImage>
#include <QBuffer>

#include "camera_stream_iface.h"

namespace Das {

class v4l2;

class Camera_Stream : public Camera_Stream_Iface
{
public:
    Camera_Stream(const std::string& device_path, uint32_t width = 0, uint32_t height = 0, int quality = -1);
    ~Camera_Stream();

    const QByteArray& get_frame() override;

    uint32_t width() const override;
    uint32_t height() const override;

    bool reinit(uint32_t width = 0, uint32_t height = 0) override;
private:
    bool open_device(const std::string &device_path);
    void close_device();

    std::string start(uint32_t width, uint32_t height);
    bool init_format(uint32_t width, uint32_t height);
    void stop();

    bool cap_frame(bool skip = false);
    bool must_convert_;

    int quality_;

    v4l2* v4l2_;
    v4lconvert_data* convert_data_;

    struct Buffer
    {
        void* start_;
        size_t length_;
    };

    std::vector<Buffer> buffers_;

    v4l2_format src_format_;
    v4l2_format dest_format_;

    QImage img_;

    QBuffer data_buffer_;
};

} // namespace Das

#endif // DAS_CAMERA_PLUGIN_CAMERA_STREAM_H
