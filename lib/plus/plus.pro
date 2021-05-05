QT -= gui
QT += sql network

CONFIG += target_predeps

TARGET = DasPlus
TEMPLATE = lib

DEFINES += DAS_LIBRARY

#QMAKE_CXXFLAGS += -save-temps

SOURCES += \
    ../Das/lib.cpp \
    das/database.cpp \
    das/authentication_info.cpp \
    das/scheme_info.cpp \
    das/structure_synchronizer_base.cpp \
    das/jwt_helper.cpp \
    das/status_helper.cpp

HEADERS +=\
    ../Das/daslib_global.h \
    ../Das/lib.h \
    das/database.h \
    das/authentication_info.h \
    das/scheme_info.h \
    das/structure_synchronizer_base.h \
    das/database_delete_info.h \
    das/jwt_helper.h \
    das/log_saver_def.h \
    das/log_saver_helper.h \
    das/status_helper.h

DESTDIR = $${OUT_PWD}/../..

#Target version
VER_MAJ = 1
VER_MIN = 5
include(../../common.pri)

LIBS += -lDas -lHelpzBase -lHelpzDBMeta -lHelpzDB -lbotan-2

unix {
    target.path = /usr/local/lib
    INSTALLS += target
}
