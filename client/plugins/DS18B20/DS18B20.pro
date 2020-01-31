TARGET = DS18B20

#Target version
VER_MAJ = 1
VER_MIN = 1

WIRINGPI_EMPTY {
    DEFINES+="NO_WIRINGPI=1"
} else {
    SOURCES += \
        one_wire.cpp

    HEADERS += \
        one_wire.h

    LIBS += -lwiringPi
}

include(../plugin.pri)
