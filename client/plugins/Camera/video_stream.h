#ifndef DAS_CAMERA_PLUGIN_VIDEO_STREAM_H
#define DAS_CAMERA_PLUGIN_VIDEO_STREAM_H

#include <vector>
#include <linux/videodev2.h>

#include <QImage>
#include <QBuffer>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct v4lconvert_data;

#ifdef __cplusplus
}
#endif /* __cplusplus */

namespace Das {

class v4l2;

class Video_Stream
{
public:
    Video_Stream(const QString& device_path, uint32_t width = 0, uint32_t height = 0, int quality = -1);
    ~Video_Stream();

    const QByteArray& param();
    const QByteArray& get_frame();

    uint32_t width() const;
    uint32_t height() const;

    void reinit(uint32_t width = 0, uint32_t height = 0);
private:
    bool open_device(const QString& device_path);
    void close_device();

    std::string start(uint32_t width, uint32_t height);
    bool init_format(uint32_t width, uint32_t height);
    void stop();

    void cap_frame();
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

    QByteArray data_, param_;
    QBuffer data_buffer_;
};

} // namespace Das

#endif // DAS_CAMERA_PLUGIN_VIDEO_STREAM_H
