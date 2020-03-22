//#include <sys/ioctl.h>
#include <sys/mman.h>

#include <QFile>
#include <QImage>
#include <QDebug>

#include "v4l2-api.h"
#include "video_stream.h"

namespace Das {

Video_Stream::Video_Stream(const QString &device_path) :
    convert_data_(nullptr)
{
    if (!open_device(device_path))
        throw std::runtime_error("Failed open device");

    const std::string error = start();
    if (!error.empty())
    {
        stop();
        close_device();

        throw std::runtime_error(error);
    }

    qsrand(time(nullptr));

    QDataStream ds(&param_, QIODevice::WriteOnly);
    ds << uint32_t(qrand()) << uint32_t(qrand());
}

Video_Stream::~Video_Stream()
{
    stop();
    close_device();
}

const QByteArray &Video_Stream::param()
{
    return param_;
}

const QByteArray &Video_Stream::get_frame()
{
    cap_frame();
    return data_;
}

bool Video_Stream::open_device(const QString &device_path)
{
    v4l2_ = new v4l2;
    if (v4l2_->open(device_path, /*useWrapper=*/true, /*is_non_block=*/false))
    {
        convert_data_ = v4lconvert_create(v4l2_->fd());
        return true;
    }

    close_device();
    return false;
}

void Video_Stream::close_device()
{
    if (!v4l2_)
        return;

    if (v4l2_->fd() >= 0)
    {
        v4lconvert_destroy(convert_data_);
        v4l2_->close();
    }

    delete v4l2_;
    v4l2_ = nullptr;
}

std::string Video_Stream::start()
{
    init_format();

    __u32 buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    if (!v4l2_->reqbufs_mmap(req, buftype, 3))
        return "Cannot capture";

    if (req.count < 2)
    {
        v4l2_->reqbufs_mmap(req, buftype);
        return "Too few buffers";
    }

    buffers_.resize(req.count);

    for (uint32_t i = 0; i < req.count; ++i)
    {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type        = buftype;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        if (-1 == v4l2_->ioctl(VIDIOC_QUERYBUF, &buf))
            return "VIDIOC_QUERYBUF";

        Buffer& buffer = buffers_[i];
        buffer.length_ = buf.length;
        buffer.start_ = v4l2_->mmap(buf.length, buf.m.offset);

        if (MAP_FAILED == buffer.start_)
            return "mmap";
    }

    for (uint32_t i = 0; i < buffers_.size(); ++i)
    {
        if (!v4l2_->qbuf_mmap(i, buftype))
            return "VIDIOC_QBUF";
    }

    if (!v4l2_->streamon(buftype))
        return "VIDIOC_STREAMON";

    return {};
}

void Video_Stream::init_format()
{
    static const struct Supported_Format
    {
        __u32 v4l2_pixfmt_;
        QImage::Format qt_pixfmt_;
    } supported_fmts[] = {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        { V4L2_PIX_FMT_RGB32, QImage::Format_RGB32 },
        { V4L2_PIX_FMT_RGB24, QImage::Format_RGB888 },
        { V4L2_PIX_FMT_RGB565X, QImage::Format_RGB16 },
        { V4L2_PIX_FMT_RGB555X, QImage::Format_RGB555 },
#else
        { V4L2_PIX_FMT_BGR32, QImage::Format_RGB32 },
        { V4L2_PIX_FMT_RGB24, QImage::Format_RGB888 },
        { V4L2_PIX_FMT_RGB565, QImage::Format_RGB16 },
        { V4L2_PIX_FMT_RGB555, QImage::Format_RGB555 },
        { V4L2_PIX_FMT_RGB444, QImage::Format_RGB444 },
#endif
        { 0, QImage::Format_Invalid }
    };

    QImage::Format dst_fmt = QImage::Format_RGB888;

    v4l2_->g_fmt_cap(src_format_);
    v4l2_->s_fmt(src_format_);

    must_convert_ = true;

    dest_format_ = src_format_;
    dest_format_.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

    for (int i = 0; supported_fmts[i].v4l2_pixfmt_; i++)
    {
        const Supported_Format& fmt = supported_fmts[i];
        if (fmt.v4l2_pixfmt_ == src_format_.fmt.pix.pixelformat)
        {
            dest_format_.fmt.pix.pixelformat = fmt.v4l2_pixfmt_;
            dst_fmt = fmt.qt_pixfmt_;
            must_convert_ = false;
            break;
        }
    }

    if (must_convert_)
    {
        v4l2_format copy = src_format_;

        v4lconvert_try_format(convert_data_, &dest_format_, &src_format_);
        // v4lconvert_try_format sometimes modifies the source format if it thinks
        // that there is a better format available. Restore our selected source
        // format since we do not want that happening.
        src_format_ = copy;
    }

#ifdef QT_DEBUG
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_UV8) qDebug() << "V4L2_PIX_FMT_UV8";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YVU410) qDebug() << "V4L2_PIX_FMT_YVU410";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YVU420) qDebug() << "V4L2_PIX_FMT_YVU420";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) qDebug() << "V4L2_PIX_FMT_YUYV";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YYUV) qDebug() << "V4L2_PIX_FMT_YYUV";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YVYU) qDebug() << "V4L2_PIX_FMT_YVYU";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY) qDebug() << "V4L2_PIX_FMT_UYVY";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_VYUY) qDebug() << "V4L2_PIX_FMT_VYUY";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV422P) qDebug() << "V4L2_PIX_FMT_YUV422P";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV411P) qDebug() << "V4L2_PIX_FMT_YUV411P";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_Y41P) qDebug() << "V4L2_PIX_FMT_Y41P";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV444) qDebug() << "V4L2_PIX_FMT_YUV444";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV555) qDebug() << "V4L2_PIX_FMT_YUV555";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV565) qDebug() << "V4L2_PIX_FMT_YUV565";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV32) qDebug() << "V4L2_PIX_FMT_YUV32";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV410) qDebug() << "V4L2_PIX_FMT_YUV410";
    if (src_format_.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) qDebug() << "V4L2_PIX_FMT_YUV420";
#endif

    QDataStream ds(&data_, QIODevice::WriteOnly);
    ds << dest_format_.fmt.pix.width << dest_format_.fmt.pix.height;

    //    m_capImage = new QImage(dest_format_.fmt.pix.width, dest_format_.fmt.pix.height, dst_fmt);
    //    m_capImage->fill(0);
}

void Video_Stream::stop()
{
    if (!v4l2_)
        return;

    __u32 buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_requestbuffers reqbufs;

    if (!v4l2_->streamoff(buftype))
        perror("VIDIOC_STREAMOFF");

    for (const Buffer& buff: buffers_)
        if (-1 == v4l2_->munmap(buff.start_, buff.length_))
            perror("munmap");

    // Free all buffers.
    v4l2_->reqbufs_mmap(reqbufs, buftype, 1);  // videobuf workaround
    v4l2_->reqbufs_mmap(reqbufs, buftype, 0);
}

void Video_Stream::cap_frame()
{
    bool again;
    v4l2_buffer buf;
    if (!v4l2_->dqbuf_mmap(buf, V4L2_BUF_TYPE_VIDEO_CAPTURE, again))
        throw std::runtime_error("dqbuf");

    if (buf.index >= buffers_.size())
        throw std::runtime_error("Bad buffer index");

    if (again)
        return;

    Buffer& buffer = buffers_[buf.index];

    int err = 0;
    if (must_convert_)
    {
        if (data_.size() != (8 + dest_format_.fmt.pix.sizeimage))
            data_.resize(8 + dest_format_.fmt.pix.sizeimage);

        err = v4lconvert_convert(convert_data_,
            &src_format_, &dest_format_,
            static_cast<uint8_t*>(buffer.start_), buf.bytesused,
            reinterpret_cast<uint8_t*>(data_.data() + 8), dest_format_.fmt.pix.sizeimage);

        if (err == -1)
            v4l2_->error(v4lconvert_get_error_message(convert_data_));
    }

    if (!must_convert_ || err < 0)
    {
        if (data_.size() != (8 + buf.bytesused))
            data_.resize(8 + buf.bytesused);

        memcpy(data_.data() + 8, buffer.start_, buf.bytesused);
    }

    v4l2_->qbuf(buf);
}

} // namespace Das
