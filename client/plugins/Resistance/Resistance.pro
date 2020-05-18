TARGET = Resistance

#Target version
VER_MAJ = 1
VER_MIN = 0

WIRINGPI_EMPTY {
    DEFINES+="NO_WIRINGPI=1"
} else {
    LIBS += -lwiringPi
}

include(../plugin.pri)
