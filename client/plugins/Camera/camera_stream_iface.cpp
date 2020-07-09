#include <QDataStream>
#include <QRandomGenerator>

#include "camera_stream_iface.h"

namespace Das {

Camera_Stream_Iface::Camera_Stream_Iface() :
    _skip_frame_count(0)
{
    QRandomGenerator rnd(time(nullptr));

    QDataStream ds(&_param, QIODevice::WriteOnly);
    ds << uint32_t(rnd.generate()) << uint32_t(rnd.generate());
}

void Camera_Stream_Iface::set_skip_frame_count(uint32_t count)
{
    _skip_frame_count = count;
}

bool Camera_Stream_Iface::is_valid() const { return _error_text.empty(); }

std::string Camera_Stream_Iface::last_error() const { return _error_text; }

const QByteArray &Camera_Stream_Iface::param()
{
    return _param;
}

} // namespace Das
