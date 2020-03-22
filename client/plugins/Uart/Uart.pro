QT += serialport

TARGET = Uart

SOURCES += \
    config.cpp

HEADERS += \
    config.h

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)
