//#include <stdio.h>
//#include <stdlib.h>
//#include <iostream>
//#include <fstream>
//#include <sstream>
#include <chrono>

//#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavformat/avio.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include "rtsp_stream.h"

namespace Das {

Codec_Item::Codec_Item() : _codec(nullptr), _codec_ctx(nullptr), _frame(nullptr), _frame_buffer(nullptr), _packet(nullptr) {}

Codec_Item::~Codec_Item()
{
    av_packet_free(&_packet);
    av_frame_free(&_frame);
    av_free(_frame_buffer);
    avcodec_free_context(&_codec_ctx);
}

// -----

RTSP_Stream::RTSP_Stream(const std::string &url, int width, int height) :
    _format_ctx(nullptr),
    _convert_ctx(nullptr),
    _index(0)
{
    _error_text = init_input_codec(url);
    if (_error_text.empty())
        _error_text = init_output_codec(width, height);

    if (_error_text.empty())
        av_read_play(_format_ctx);
}

RTSP_Stream::~RTSP_Stream()
{
    if (_format_ctx)
        av_read_pause(_format_ctx);

    avformat_close_input(&_format_ctx);
    avformat_free_context(_format_ctx);
    sws_freeContext(_convert_ctx);
}

uint32_t RTSP_Stream::width() const { return _out._codec_ctx ? _out._codec_ctx->width : 0; }
uint32_t RTSP_Stream::height() const { return _out._codec_ctx ? _out._codec_ctx->height : 0; }

bool RTSP_Stream::reinit(uint32_t width, uint32_t height)
{
    if (!is_valid())
        return false;

    av_read_pause(_format_ctx);

    if (_convert_ctx)
    {
        sws_freeContext(_convert_ctx);
        _convert_ctx = nullptr;

        av_free(_out._frame);
        av_free(_out._frame_buffer);
    }

    avcodec_free_context(&_out._codec_ctx);

    _error_text = init_output_codec_base(width, height);
    if (!_error_text.empty())
        return false;

    av_read_play(_format_ctx);
    return true;
}

const QByteArray &RTSP_Stream::get_frame()
{
    cap_frame();
    return _data;
}

/*static*/ int RTSP_Stream::check_interrupt(void *ptr)
{
    const auto now = std::chrono::system_clock::now();
    return ptr && now - static_cast<RTSP_Stream*>(ptr)->_last_frame > std::chrono::milliseconds(5500);
}

std::string RTSP_Stream::init_input_codec(const std::string &url)
{
    AVDictionary *opts = nullptr;
    _error_code = av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    if (_error_code < 0)
        return "Unable to set rtsp_transport options.";

    _format_ctx = avformat_alloc_context();

    _format_ctx->interrupt_callback.opaque = this;
    _format_ctx->interrupt_callback.callback = &RTSP_Stream::check_interrupt;

    _last_frame = std::chrono::system_clock::now();
    _error_code = avformat_open_input(&_format_ctx, url.c_str(), nullptr, &opts);
    if (_error_code != 0)
    {
        av_dict_free(&opts);
        return "Unable to open stream url";
    }

    _error_code = avformat_find_stream_info(_format_ctx, nullptr);
    if (_error_code < 0)
        return "Unable to find stream info";

    //search video stream
    _video_stream_index = _format_ctx->nb_streams;
    for (uint32_t i = 0; i < _format_ctx->nb_streams; ++i)
    {
        if (_video_stream_index >= _format_ctx->nb_streams
                && _format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _format_ctx->streams[i]->discard = AVDISCARD_NONKEY;
            _video_stream_index = i;
        }
        else
        {
            _format_ctx->streams[i]->discard = AVDISCARD_ALL;
        }
    }

    if (_video_stream_index >= _format_ctx->nb_streams)
        return "Unable to find video stream";

    // Get the codec
    _in._codec = avcodec_find_decoder(_format_ctx->streams[_video_stream_index]->codecpar->codec_id);
    if (!_in._codec)
        return "Unable to find decoder";

    // Add this to allocate the context by codec
    _in._codec_ctx = avcodec_alloc_context3(_in._codec);
    if (!_in._codec_ctx)
        return "Could not allocate stream codec context";

    avcodec_parameters_to_context(_in._codec_ctx, _format_ctx->streams[_video_stream_index]->codecpar);

    _error_code = avcodec_open2(_in._codec_ctx, _in._codec, NULL);
    if (_error_code < 0)
        return "Unable to open decoder";

    int size = av_image_get_buffer_size(_in._codec_ctx->pix_fmt, _in._codec_ctx->width, _in._codec_ctx->height, 16);
    _in._frame_buffer = (uint8_t*) av_malloc(size);
    _in._frame = av_frame_alloc();
    _error_code = av_image_fill_arrays(_in._frame->data, _in._frame->linesize, _in._frame_buffer, _in._codec_ctx->pix_fmt, _in._codec_ctx->width, _in._codec_ctx->height, 1);
    if (_error_code < 0)
        return "Unable to init frame";

    _in._packet = av_packet_alloc();

    return {};
}

std::string RTSP_Stream::init_output_codec(int width, int height)
{
    _out._codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!_out._codec)
        return "MJpeg codec not found";

    _out._packet = av_packet_alloc();
    if (!_out._packet)
        return "Could not allocate MJpeg packet";

    return init_output_codec_base(width, height);
}

std::string RTSP_Stream::init_output_codec_base(int width, int height)
{
    _out._codec_ctx = avcodec_alloc_context3(_out._codec);
    if (!_out._codec_ctx)
        return "Could not allocate MJpeg codec context";

    if (width <= 0)
        width = _in._codec_ctx->width;
    if (height <= 0)
        height = _in._codec_ctx->height;

    _out._codec_ctx->width = width;
    _out._codec_ctx->height = height;

    _out._codec_ctx->bit_rate = _in._codec_ctx->bit_rate;
    _out._codec_ctx->time_base.num = _in._codec_ctx->time_base.num;
    _out._codec_ctx->time_base.den = _in._codec_ctx->time_base.den;

    _out._codec_ctx->pix_fmt = _in._codec_ctx->pix_fmt;

    auto* fmt = _out._codec->pix_fmts;
    while (*fmt != _out._codec_ctx->pix_fmt)
    {
        if (*fmt++ == AV_PIX_FMT_NONE)
        {
            _out._codec_ctx->pix_fmt = *_out._codec->pix_fmts;
            break;
        }
    }

    if (avcodec_open2(_out._codec_ctx, _out._codec, NULL) < 0)
        return "Could not open MJpeg codec";

    if (_out._codec_ctx->width != _in._codec_ctx->width
            || _out._codec_ctx->height != _in._codec_ctx->height)
    {
        int size = av_image_get_buffer_size(_out._codec_ctx->pix_fmt, _out._codec_ctx->width, _out._codec_ctx->height, 16);
        _out._frame_buffer = (uint8_t*) av_malloc(size);
        _out._frame = av_frame_alloc();
        _error_code = av_image_fill_arrays(_out._frame->data, _out._frame->linesize, _out._frame_buffer, _out._codec_ctx->pix_fmt, _out._codec_ctx->width, _out._codec_ctx->height, 1);
        if (_error_code < 0)
            return "Unable to init picture";

        _convert_ctx = sws_getContext(_in._codec_ctx->width, _in._codec_ctx->height, _in._codec_ctx->pix_fmt,
                                      _out._codec_ctx->width, _out._codec_ctx->height, _out._codec_ctx->pix_fmt,
                                      SWS_BICUBIC, NULL, NULL, NULL);
        if (!_convert_ctx)
            return "Could not allocate convert context";
    }

    return {};
}

void RTSP_Stream::cap_frame()
{
    if (!is_valid())
        return;

    int err_code = 0, check = 0;

    bool break_flag = false;

    if (_skip_frame_count < 2)
        _skip_frame_count = 2;

    _last_frame = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point now;

    while (av_read_frame(_format_ctx, _in._packet) >= 0 && !break_flag)
    {
        if (_in._packet->stream_index == static_cast<int>(_video_stream_index))
        {
            check = 0;
            err_code = avcodec_receive_frame(_in._codec_ctx, _in._frame);
            if (err_code == 0)
                check = 1;
            if (err_code == AVERROR(EAGAIN))
                err_code = 0;
            if (err_code == 0)
                err_code = avcodec_send_packet(_in._codec_ctx, _in._packet);

            if (check)
            {
                now = std::chrono::system_clock::now();

                if (now - _last_frame > std::chrono::milliseconds(10))
                {
                    if (_skip_frame_count <= 0)
                    {
                        save_jpeg();
                        break_flag = true;
                    }
                    else
                        --_skip_frame_count;
                }

                _last_frame = now;
            }
        }
        av_packet_unref(_in._packet);
    }

    _skip_frame_count = 0;
}

void RTSP_Stream::save_jpeg()
{
    _out._codec_ctx->qmin = _out._codec_ctx->qmax = 3;
    _out._codec_ctx->mb_lmin = _out._codec_ctx->qmin * FF_QP2LAMBDA;
    _out._codec_ctx->mb_lmax = _out._codec_ctx->qmax * FF_QP2LAMBDA;
    _out._codec_ctx->flags |= AV_CODEC_FLAG_QSCALE;

    AVFrame* frame;

    if (_convert_ctx)
    {
        frame = _out._frame;
        sws_scale(_convert_ctx, _in._frame->data, _in._frame->linesize, 0,
                  _in._codec_ctx->height, _out._frame->data, _out._frame->linesize);
    }
    else
        frame = _in._frame;

    frame->quality = 1;
    frame->pts = _index++;
    frame->width = _out._codec_ctx->width;
    frame->height = _out._codec_ctx->height;
    frame->format = _out._codec_ctx->pix_fmt;

    int ret = avcodec_send_frame(_out._codec_ctx, frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a frame for encoding\n");
        return;
    }

    int pos = 0;
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(_out._codec_ctx, _out._packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0)
        {
            fprintf(stderr, "Error during encoding\n");
            break;
        }

        if (_data.size() < (pos + _out._packet->size))
            _data.resize(pos + _out._packet->size);
        memcpy(_data.data() + pos, _out._packet->data, _out._packet->size);
        pos += _out._packet->size;

        av_packet_unref(_out._packet);
    }

    if (_data.size() > pos)
        _data.resize(pos);
}

} // namespace Das
