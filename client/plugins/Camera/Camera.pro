TARGET = Camera

SOURCES += \
#    encoder.cpp \
#    recorder.cpp \
    camera_stream.cpp \
    camera_stream_iface.cpp \
    camera_thread.cpp \
    rtsp_stream.cpp \
    v4l2-api.cpp \
    config.cpp \
    stream/stream_client_thread.cpp \
    stream/stream_controller.cpp

HEADERS += \
#    encoder.h \
#    recorder.h \
    camera_stream.h \
    camera_stream_iface.h \
    camera_thread.h \
    rtsp_stream.h \
    v4l2-api.h \
    config.h \
    stream/stream_client_thread.h \
    stream/stream_controller.h

LIBS += -lboost_system
LIBS += -lv4l1 -lv4l2 -lv4lconvert
LIBS += -lavformat -lavcodec -lavutil -lswscale
LIBS += -lHelpzNetwork

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)
QT += gui
