TARGET = Router

#Target version
VER_MAJ = 1
VER_MIN = 0

include(../plugin.pri)

HEADERS += \
    system_informator.h \
    wpa_supplicant_config.h

SOURCES += \
    system_informator.cpp \
    wpa_supplicant_config.cpp
