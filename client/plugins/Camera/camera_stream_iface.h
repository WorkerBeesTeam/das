#ifndef DAS_CAMERA_PLUGIN_CAMERA_STREAM_INTERFACE_H
#define DAS_CAMERA_PLUGIN_CAMERA_STREAM_INTERFACE_H

#include <QByteArray>

namespace Das {

class Camera_Stream_Iface
{
public:
    Camera_Stream_Iface();

    void set_skip_frame_count(uint32_t count);

    bool is_valid() const;
    std::string last_error() const;

    const QByteArray& param() const;

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    virtual bool reinit(uint32_t width = 0, uint32_t height = 0) = 0;
    virtual const QByteArray& get_frame() = 0;
protected:
    uint32_t _skip_frame_count;

    QByteArray _data;
    std::string _error_text;
private:
    QByteArray _param;
};

} // namespace Das

#endif // DAS_CAMERA_PLUGIN_CAMERA_STREAM_INTERFACE_H
