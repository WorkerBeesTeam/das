QT += dbus
TARGET = Gatt

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)

HEADERS += \
    gatt_characteristic_receiver.h

SOURCES += \
    gatt_characteristic_receiver.cpp
