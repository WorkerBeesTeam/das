QT -= gui
QT += dbus

CONFIG += target_predeps

TARGET = DasDbus
TEMPLATE = lib

DEFINES += DAS_LIBRARY

#QMAKE_CXXFLAGS += -save-temps

SOURCES += \
    ../Das/lib.cpp \
    dbus_common.cpp \
    dbus_object_base.cpp \
    dbus_interface.cpp

HEADERS +=\
    ../Das/daslib_global.h \
    ../Das/lib.h \
    dbus_common.h \
    dbus_object_base.h \
    dbus_interface.h

DESTDIR = $${OUT_PWD}/../..

#Target version
VER_MAJ = 1
VER_MIN = 1
include(../../common.pri)

unix {
    target.path = /usr/local/lib
    INSTALLS += target
}
