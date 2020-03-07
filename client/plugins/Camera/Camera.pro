TARGET = Camera

SOURCES += \
#    encoder.cpp \
#    recorder.cpp \
    v4l2-api.cpp \
    config.cpp \
    video_stream.cpp

HEADERS += \
#    encoder.h \
#    recorder.h \
    v4l2-api.h \
    config.h \
    video_stream.h

LIBS += -lv4l1 -lv4l2 -lv4lconvert

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)
QT += gui
