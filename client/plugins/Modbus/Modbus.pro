QT += serialport serialbus
TARGET = Modbus

#Target version
VER_MAJ = 1
VER_MIN = 2

SOURCES += \
    modbus_plugin_base.cpp \
    config.cpp

HEADERS += \
    modbus_plugin_base.h \
    config.h

unix {
    CONFIG(debug, debug|release): QT += dbus
}

include(../plugin.pri)
