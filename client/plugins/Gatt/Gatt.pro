TARGET = Gatt

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)

LIBS += -lgattlib

HEADERS += \
    gatt_common.h \
    gatt_finder.h \
    gatt_notification_listner.h

SOURCES += \
    gatt_common.cpp \
    gatt_finder.cpp \
    gatt_notification_listner.cpp
