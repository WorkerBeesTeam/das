#ifndef DAS_CAMERA_PLUGIN_RTSP_STREAM_H
#define DAS_CAMERA_PLUGIN_RTSP_STREAM_H

#include <chrono>

#include "camera_stream_iface.h"

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVFormatContext;
struct SwsContext;

namespace Das {

struct Codec_Item
{
    Codec_Item();
    ~Codec_Item();

    AVCodec *_codec;
    AVCodecContext *_codec_ctx;

    AVFrame* _frame;
    uint8_t* _frame_buffer;

    AVPacket* _packet;
};

class RTSP_Stream : public Camera_Stream_Iface
{
public:
    explicit RTSP_Stream(const std::string& url, int width = 0, int height = 0);

    ~RTSP_Stream();

    uint32_t width() const override;
    uint32_t height() const override;

    bool reinit(uint32_t width = 0, uint32_t height = 0) override;

    const QByteArray& get_frame() override;
private:

    static int check_interrupt(void* ptr);

    std::string init_input_codec(const std::string& url);
    std::string init_output_codec(int width = 0, int height = 0);
    std::string init_output_codec_base(int width = 0, int height = 0);
    void cap_frame();
    void save_jpeg();

    AVFormatContext* _format_ctx;
    uint32_t _video_stream_index;

    Codec_Item _in, _out;

    SwsContext* _convert_ctx;

    int _error_code, _index;

    std::chrono::system_clock::time_point _last_frame;
};

} // namespace Das

#endif // DAS_CAMERA_PLUGIN_RTSP_STREAM_H
