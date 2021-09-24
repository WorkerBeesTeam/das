QT += serialport serialbus
TARGET = Modbus

#Target version
VER_MAJ = 1
VER_MIN = 2

SOURCES += \
    modbus_controller.cpp \
    modbus_log.cpp \
    modbus_pack.cpp \
    modbus_thread.cpp \
    modbus_pack_builder.cpp \
    modbus_pack_queue.cpp \
    modbus_pack_read_manager.cpp \
    modbus_plugin_base.cpp \
    config.cpp

HEADERS += \
    modbus_controller.h \
    modbus_log.h \
    modbus_pack.h \
    modbus_thread.h \
    modbus_pack_builder.h \
    modbus_pack_queue.h \
    modbus_pack_read_manager.h \
    modbus_plugin_base.h \
    config.h

unix {
    CONFIG(debug, debug|release): QT += dbus
}

include(../plugin.pri)
