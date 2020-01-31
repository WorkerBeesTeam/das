TARGET = WiringPi
# TODO: rename to GPIO

#Target version
VER_MAJ = 1
VER_MIN = 1

WIRINGPI_EMPTY {
    DEFINES+="NO_WIRINGPI=1"
} else {
    LIBS += -lwiringPi
}

include(../plugin.pri)
