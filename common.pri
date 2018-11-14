MOC_DIR = $${OUT_PWD}/build/moc
OBJECTS_DIR = $${OUT_PWD}/build/obj
CONFIG += object_parallel_to_source

android {
    CONFIG += c++14
} else {
    CONFIG += c++1z
}

#QMAKE_CXXFLAGS += -std:c++latest

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

linux-g++* {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
}

CONFIG (debug, debug|release) {
    CONFIG += warn_on

    CONFIG ~= s/-O[0123s]//g
    CONFIG += -O0
    QMAKE_CXXFLAGS += -O0
#    QMAKE_CXXFLAGS += -Wextra -Weffc++ -Wold-style-cast -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code
}
CONFIG (release, debug|release) {
    QMAKE_CFLAGS_WARN_ON -= -Wunused
    QMAKE_CXXFLAGS_WARN_ON -= -Wunused
}

isEmpty(DESTDIR): DESTDIR = $${OUT_PWD}/..

INCLUDEPATH += $$PWD/lib $${DESTDIR}/helpz/include
LIBS += -L$${DESTDIR} -L$${DESTDIR}/helpz

include(helpz/helpz_version.pri)
