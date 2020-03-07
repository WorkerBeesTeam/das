
#include <QDebug>
#include "QImage"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern "C" {
#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <libavutil/error.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include "encoder.h"
#include "stdio.h"

#define AV_CODEC_FLAG_GLOBAL_HEADER (1 << 22)
#define CODEC_FLAG_GLOBAL_HEADER AV_CODEC_FLAG_GLOBAL_HEADER
#define AVFMT_RAWPICTURE 0x0020

bool printError(const char* msg, int err = 0)
{
    char buff[128];
    if (err != 0)
        av_strerror(err, buff, 128);

    fprintf(stderr, msg, buff);
    return false;
}

QVideoOutput::QVideoOutput(QObject *parent)
    : QObject(parent)
    , swsContext(0x0)
    , formatContext(0x0)
    , outputFormat(0x0)
    , videoStream(0x0)
    , videoCodec(0x0)
    , frame( nullptr )
    , swsFlags(SWS_BICUBIC)
    , streamFrameRate(12)              // 25 images/s
    , width(640)
    , height(480)
    , opened(false)
{
//    av_register_all();
//    avcodec_register_all();
}

QVideoOutput::~QVideoOutput()
{
}

void QVideoOutput::setResolution(int inWidth, int inHeight)
{
    Q_ASSERT(inWidth%2  == 0);
    Q_ASSERT(inHeight%2 == 0);
    width  = inWidth;
    height = inHeight;
}

AVStream *QVideoOutput::addStream(AVFormatContext * inFormatContext,
                                  AVCodec **codec) const
{
    AVCodecID codec_id = AV_CODEC_ID_H264; // inFormatContext->oformat->video_codec

    AVCodecContext *c;
//    AVCodecParameters *c;
    AVStream *st;
    // find the encoder
    *codec = avcodec_find_encoder( codec_id );
    if (!(*codec))
        return 0x0;
    st = avformat_new_stream(inFormatContext, *codec);
    if (!st)
        return (AVStream *)printError("Could not allocate stream\n");
    st->id = inFormatContext->nb_streams-1;
    c = st->codec;
//    c = st->codecpar;
    st->time_base.den = streamFrameRate;
    st->time_base.num = 1;

    avcodec_get_context_defaults3(c, *codec);
    qDebug() << "Codec id" << c->codec_id << codec_id;
    c->codec_id = codec_id;
//    c->bit_rate = width * height/*400000*/;
    c->bit_rate = width * height;
    c->width    = width;
    c->height   = height;
    // timebase: This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identical to 1.
    c->time_base.den = streamFrameRate;
    c->time_base.num = 1;
    c->gop_size      = 10; // emit one intra frame every twelve frames at most
    c->pix_fmt       = AV_PIX_FMT_YUV420P;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
    {
        // just for testing, we also add B frames
        c->max_b_frames = 2;
    }
    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
    {
        // Needed to avoid using macroblocks in which some coeffs overflow.
        // This does not happen with normal video, it just happens here as
        // the motion of the chroma plane does not match the luma plane.
        c->mb_decision = 2;
    }
    if (c->codec_id == AV_CODEC_ID_H264)
        qDebug() << "Codec is H264";

    // Some formats want stream headers to be separate.
    if (inFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    return st;
}

bool QVideoOutput::openVideo(AVCodec *codec, AVStream *stream)
{
    int ret;
    AVCodecContext *c = stream->codec;
    // open the codec
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0)
        return printError("Could not open video codec: %s\n", ret);

    // allocate and init a re-usable frame
    frame = av_frame_alloc();
    if (!frame)
        return printError("Could not allocate video frame\n");
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                         c->pix_fmt, 32);

    // Allocate the encoded raw picture.
//    ret = avpicture_alloc(&dstPicture, c->pix_fmt, c->width, c->height);
    if (ret < 0)
        return printError("Could not allocate picture: %s\n", ret);

    // copy data and linesize picture pointers to frame
//    *((AVPicture *)frame) = dstPicture;
    return true;
}

bool QVideoOutput::writeVideoFrame(int width, int height, int format,
                                   const uint8_t* bits, int bytesPerLine,
                                   AVFormatContext * inFormatContext,
                                   AVStream *stream)
{
    int ret;
    AVCodecContext *c = stream->codec;
    if (c->pix_fmt != AV_PIX_FMT_RGBA)
    {
        // as we only use RGBA picture, we must convert it
        // to the codec pixel format if needed
        if (!swsContext)
        {
            swsContext = sws_getContext(width, height,
                                        (AVPixelFormat)format,
                                        c->width, c->height,
                                        c->pix_fmt,
                                        swsFlags, NULL, NULL, NULL);
            if (!swsContext)
                return printError("Could not initialize the conversion context\n");
        }

        const uint8_t *srcplanes[3];
        srcplanes[0] = bits;
        srcplanes[1] = 0;
        srcplanes[2] = 0;

        int srcstride[3];
        srcstride[0] = bytesPerLine;
        srcstride[1] = 0;
        srcstride[2] = 0;

        sws_scale(swsContext, srcplanes, srcstride, 0, c->height, frame->data, frame->linesize);
    }
    if (inFormatContext->oformat->flags & AVFMT_RAWPICTURE)
    {
        // Raw video case - directly store the picture in the packet
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.flags        |= AV_PKT_FLAG_KEY;
        pkt.stream_index  = stream->index;
        pkt.data          = frame->data[0];
        pkt.size          = sizeof(AVPicture);
        ret = av_interleaved_write_frame(inFormatContext, &pkt);
    }
    else
    {
        // encode the image
        AVPacket pkt;
        int gotOutput;
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        ret = avcodec_encode_video2(c, &pkt, frame, &gotOutput);
        if (ret < 0)
            return printError("Error encoding video frame: %s\n", ret);

        // If size is zero, it means the image was buffered.
        if (gotOutput)
        {
            if (c->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;
            pkt.stream_index = stream->index;
            // Write the compressed frame to the media file.
            ret = av_interleaved_write_frame(inFormatContext, &pkt);
        }
        else
        {
            ret = 0;
        }
    }
    if (ret != 0)
        return printError("Error while writing video frame: %s\n", ret);

    frameCount++;
    return true;
}

bool QVideoOutput::openMediaFile(int width, int height, int frameRate, const QString & filename)
{
    setResolution(width, height);
    streamFrameRate = frameRate;

    // allocate the output media context
    avformat_alloc_output_context2(&formatContext, NULL, NULL, qPrintable(filename));
    if (!formatContext)
    {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&formatContext, NULL, "mpeg", qPrintable(filename));
        if (!formatContext)
            return false;
    }
    outputFormat = formatContext->oformat;
    // Add the video streams using the default format codecs
    // and initialize the codecs.
    videoStream = NULL;

    if (outputFormat->video_codec != AV_CODEC_ID_NONE)
        videoStream = addStream(formatContext, &videoCodec);
    // Now that all the parameters are set, we can open the audio and
    // video codecs and allocate the necessary encode buffers.
    if (videoStream)
        if (!openVideo(videoCodec, videoStream))
            return false;
    av_dump_format(formatContext, 0, qPrintable(filename), 1);

    int ret = 0;
    // open the output file, if needed
    if (!(outputFormat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&formatContext->pb, qPrintable(filename), AVIO_FLAG_WRITE);
        if (ret < 0)
            return printError(qPrintable("Could not open '" + filename + "': %s\n"), ret);
    }
    // Write the stream header, if any.
    ret = avformat_write_header(formatContext, NULL);
    if (ret < 0)
        return printError("Error occurred when opening output file: %s\n", ret);

    if (frame)
        frame->pts = 0;

    opened = true;
    return true;
}

bool QVideoOutput::newFrame(const QImage & image)
{
//    if(image.format()!=QImage::Format_RGB32	&& image.format() != QImage::Format_ARGB32)
//        return printError("Wrong image format\n");

    int format = image.format() == QImage::Format_RGB888 ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_RGBA;

    writeVideoFrame(image.width(), image.height(), format,
                    image.bits(), image.bytesPerLine(),
                    formatContext, videoStream);

    frame->pts += av_rescale_q(1, videoStream->codec->time_base,
                                  videoStream->time_base);
    return true;
}

//bool QVideoOutput::newFrame(const QVideoFrame &f)
//{
//    int format = f.pixelFormat() == QVideoFrame::Format_RGB24 ?
//                AV_PIX_FMT_BGR24 : AV_PIX_FMT_BGRA;

//    writeVideoFrame(f.width(), f.height(), format,
//                    f.bits(), f.bytesPerLine(),
//                    formatContext, videoStream);

//    frame->pts += av_rescale_q(1, videoStream->codec->time_base,
//                                  videoStream->time_base);
//    return true;
//}

bool QVideoOutput::closeMediaFile()
{
    av_write_trailer(formatContext);
    if (videoStream)
        closeVideo(videoStream);
    if (swsContext)
    {
        sws_freeContext(swsContext);
        swsContext = 0x0;
    }

    for (unsigned int i = 0; i < formatContext->nb_streams; i++)
    {
        av_freep(&formatContext->streams[i]->codec);
        av_freep(&formatContext->streams[i]);
    }
    if (!(outputFormat->flags & AVFMT_NOFILE))
        avio_close(formatContext->pb);
    av_free(formatContext);

    opened = false;
    return true;
}

void QVideoOutput::closeVideo(AVStream *stream)
{
    avcodec_close(stream->codec);
    av_free(frame->data[0]);
//    av_free(dstPicture.data[0]);
    av_free(frame);
}
