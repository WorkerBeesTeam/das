#include <QDataStream>
#include <QRandomGenerator>

#include "camera_stream_iface.h"

namespace Das {

Camera_Stream_Iface::Camera_Stream_Iface() :
    _skip_frame_count(0)
{
    QDataStream ds(&_param, QIODevice::WriteOnly);
    ds << uint32_t(QRandomGenerator::system()->generate()) << uint32_t(QRandomGenerator::system()->generate());
}

void Camera_Stream_Iface::set_skip_frame_count(uint32_t count)
{
    _skip_frame_count = count;
}

bool Camera_Stream_Iface::is_valid() const { return _error_text.empty(); }

std::string Camera_Stream_Iface::last_error() const { return _error_text; }

const QByteArray &Camera_Stream_Iface::param() const
{
    return _param;
}

} // namespace Das
