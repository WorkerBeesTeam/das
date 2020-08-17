//#include <sys/ioctl.h>
#include <sys/mman.h>

#include <QFile>
#include <QDebug>

#include "v4l2-api.h"
#include "camera_stream.h"

namespace Das {

Camera_Stream::Camera_Stream(const std::string &device_path, uint32_t width, uint32_t height, int quality) :
    quality_(quality),
    convert_data_(nullptr),
    data_buffer_(&_data)
{
    if (!open_device(device_path))
        throw std::runtime_error("Failed open device");

    data_buffer_.open(QIODevice::WriteOnly);

    const std::string error = start(width, height);
    if (!error.empty())
    {
        stop();
        close_device();
        throw std::runtime_error(error);
    }
}

Camera_Stream::~Camera_Stream()
{
    stop();
    close_device();
}

const QByteArray &Camera_Stream::get_frame()
{
    while(_skip_frame_count > 0)
    {
        if (cap_frame(false))
            --_skip_frame_count;
    }

    cap_frame();

    data_buffer_.seek(0);
    if (!img_.save(&data_buffer_, "JPEG", quality_))
        qWarning() << "Failed save frame";
    return _data;
}

uint32_t Camera_Stream::width() const
{
    return dest_format_.fmt.pix.width;
}

uint32_t Camera_Stream::height() const
{
    return dest_format_.fmt.pix.height;
}

bool Camera_Stream::reinit(uint32_t width, uint32_t height)
{
    stop();

    _error_text = start(width, height);
    return is_valid();
}

bool Camera_Stream::open_device(const std::string &device_path)
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

void Camera_Stream::close_device()
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

    data_buffer_.close();
}

std::string Camera_Stream::start(uint32_t width, uint32_t height)
{
    if (!init_format(width, height))
        return "Failed init format";

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

std::string fcc2s(unsigned int val)
{
    std::string s;

    s += val & 0x7f;
    s += (val >> 8) & 0x7f;
    s += (val >> 16) & 0x7f;
    s += (val >> 24) & 0x7f;
    if (val & (1 << 31))
        s += "-BE";
    return s;
}

bool Camera_Stream::init_format(uint32_t width, uint32_t height)
{
//    static const struct Supported_Format
//    {
//        __u32 v4l2_pixfmt_;
//        QImage::Format qt_pixfmt_;
//    } supported_fmts[] = {
//#if Q_BYTE_ORDER == Q_BIG_ENDIAN
//        { V4L2_PIX_FMT_RGB32, QImage::Format_RGB32 },
//        { V4L2_PIX_FMT_RGB24, QImage::Format_RGB888 },
//        { V4L2_PIX_FMT_RGB565X, QImage::Format_RGB16 },
//        { V4L2_PIX_FMT_RGB555X, QImage::Format_RGB555 },
//#else
//        { V4L2_PIX_FMT_BGR32, QImage::Format_RGB32 },
//        { V4L2_PIX_FMT_RGB24, QImage::Format_RGB888 },
//        { V4L2_PIX_FMT_RGB565, QImage::Format_RGB16 },
//        { V4L2_PIX_FMT_RGB555, QImage::Format_RGB555 },
//        { V4L2_PIX_FMT_RGB444, QImage::Format_RGB444 },
//#endif
//        { 0, QImage::Format_Invalid }
//    };

    v4l2_->g_fmt_cap(src_format_);

    uint32_t pixel_format = src_format_.fmt.pix.pixelformat;

    if (!width || !height)
    {
        Frame_Size size = v4l2_->get_max_resolution();
        width = size.width_;
        height = size.height_;
        pixel_format = size.pixel_format_;
    }

    if (width != src_format_.fmt.pix.width
        || height != src_format_.fmt.pix.height)
    {
        src_format_.fmt.pix.width = width;
        src_format_.fmt.pix.height = height;
        src_format_.fmt.pix.pixelformat = pixel_format;
        src_format_.fmt.pix.bytesperline = 0;
        src_format_.fmt.pix.sizeimage = 0;
    }

    v4l2_->s_fmt(src_format_);

    must_convert_ = true;

    dest_format_ = src_format_;
    dest_format_.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

//    for (int i = 0; supported_fmts[i].v4l2_pixfmt_; i++)
//    {
//        const Supported_Format& fmt = supported_fmts[i];
//        if (fmt.v4l2_pixfmt_ == src_format_.fmt.pix.pixelformat)
//        {
//            dest_format_.fmt.pix.pixelformat = fmt.v4l2_pixfmt_;
////            dst_fmt = fmt.qt_pixfmt_;
//            must_convert_ = false;
//            break;
//        }
//    }

    if (must_convert_)
    {
        v4l2_format copy = src_format_;

        if (width && height
            && (width != dest_format_.fmt.pix.width
                || height != dest_format_.fmt.pix.height))
        {
            dest_format_.fmt.pix.width = width;
            dest_format_.fmt.pix.height = height;
            dest_format_.fmt.pix.bytesperline = 0;
            dest_format_.fmt.pix.sizeimage = 0;
        }

        v4lconvert_try_format(convert_data_, &dest_format_, &src_format_);
        // v4lconvert_try_format sometimes modifies the source format if it thinks
        // that there is a better format available. Restore our selected source
        // format since we do not want that happening.
        src_format_ = copy;

//        qDebug().nospace() << "Convert from "
//                 << fcc2s(src_format_.fmt.pix.pixelformat).c_str() << ' ' << src_format_.fmt.pix.width << 'x' << src_format_.fmt.pix.height << " size " << src_format_.fmt.pix.sizeimage
//                 << " to "
//                 << fcc2s(dest_format_.fmt.pix.pixelformat).c_str() << ' ' << dest_format_.fmt.pix.width << 'x' << dest_format_.fmt.pix.height << " size " << dest_format_.fmt.pix.sizeimage;
    }

    QImage::Format dst_fmt = QImage::Format_RGB888;
    img_ = QImage(dest_format_.fmt.pix.width, dest_format_.fmt.pix.height, dst_fmt);
    img_.fill(0);

    return img_.sizeInBytes() == dest_format_.fmt.pix.sizeimage;
}

void Camera_Stream::stop()
{
    if (!v4l2_)
        return;

    __u32 buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_requestbuffers reqbufs;

    if (!v4l2_->streamoff(buftype))
        perror("VIDIOC_");

    for (const Buffer& buff: buffers_)
        if (-1 == v4l2_->munmap(buff.start_, buff.length_))
            perror("munmap");

    // Free all buffers.
    v4l2_->reqbufs_mmap(reqbufs, buftype, 1);  // videobuf workaround
    v4l2_->reqbufs_mmap(reqbufs, buftype, 0);
}

bool Camera_Stream::cap_frame(bool skip)
{
    bool again;
    v4l2_buffer buf;
    if (!v4l2_->dqbuf_mmap(buf, V4L2_BUF_TYPE_VIDEO_CAPTURE, again))
        throw std::runtime_error("dqbuf");

    if (buf.index >= buffers_.size())
        throw std::runtime_error("Bad buffer index");

    if (again)
        return false;

    if (!skip)
    {
        Buffer& buffer = buffers_[buf.index];

        int err = 0;
        if (must_convert_)
        {
            err = v4lconvert_convert(convert_data_,
                &src_format_, &dest_format_,
                static_cast<uint8_t*>(buffer.start_), buf.bytesused,
                img_.bits(), dest_format_.fmt.pix.sizeimage);

            if (err == -1)
                v4l2_->error(v4lconvert_get_error_message(convert_data_));
        }

        if (!must_convert_ || err < 0)
            memcpy(img_.bits(), buffer.start_, buf.bytesused);
    }

    v4l2_->qbuf(buf);
    return true;
}

} // namespace Das
