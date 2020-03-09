TARGET = Camera

SOURCES += \
#    encoder.cpp \
#    recorder.cpp \
    v4l2-api.cpp \
    config.cpp \
    video_stream.cpp \
    stream/stream_client.cpp \
    stream/stream_client_thread.cpp \
    stream/stream_controller.cpp

HEADERS += \
#    encoder.h \
#    recorder.h \
    v4l2-api.h \
    config.h \
    video_stream.h \
    stream/stream_client.h \
    stream/stream_client_thread.h \
    stream/stream_controller.h

LIBS += -lv4l1 -lv4l2 -lv4lconvert
LIBS += -lboost_system
LIBS += -lHelpzNetwork

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)
QT += gui
