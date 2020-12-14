QT += serialport

TARGET = Uart

linux {
    CONFIG += link_pkgconfig
    PKGCONFIG += lua53
}

SOURCES += \
    config.cpp \
    lua_engine.cpp \
    uart_thread.cpp

HEADERS += \
    config.h \
    lua_engine.h \
    uart_thread.h

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)
