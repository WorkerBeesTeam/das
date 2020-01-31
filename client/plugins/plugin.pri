QT += core
QT -= gui

TARGET = $${TARGET}Plugin
TEMPLATE = lib

DEFINES += DAS_PLUGIN_LIBRARY

DESTDIR = $${OUT_PWD}/../../..

include(../../common.pri)

SOURCES += \
    plugin.cpp

HEADERS += \
    ../plugin_global.h \
    plugin.h

OTHER_FILES = checkerinfo.json

LIBS += -lDas

unix {
    CONFIG += unversioned_libname unversioned_soname

    target.path = /opt/das/plugins
    INSTALLS += target
}
