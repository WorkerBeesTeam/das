#ifndef DAS_CAMERA_PLUGIN_VIDEO_STREAM_H
#define DAS_CAMERA_PLUGIN_VIDEO_STREAM_H

#include <vector>
#include <linux/videodev2.h>

#include <QString>

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
    Video_Stream(const QString& device_path);
    ~Video_Stream();

    const QByteArray& param();
    const QByteArray& get_frame();

private:
    bool open_device(const QString& device_path);
    void close_device();

    std::string start();
    void init_format();
    void stop();

    void cap_frame();
    bool must_convert_;

    QByteArray data_, param_;
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
};

} // namespace Das

#endif // DAS_CAMERA_PLUGIN_VIDEO_STREAM_H
