QT += core network dbus sql
QT -= gui widgets

TARGET = DasServer
CONFIG += console
CONFIG -= app_bundle

#QMAKE_CXXFLAGS += -save-temps

TEMPLATE = app

#Target version
VER_MAJ = 1
VER_MIN = 5
include(../common.pri)

LIBS += -lDas -lDasPlus -lDasDbus -lHelpzBase -lHelpzService -lHelpzNetwork -lHelpzDBMeta -lHelpzDB -lHelpzDTLS -lbotan-2 -lboost_system -lboost_thread

SOURCES += main.cpp \
    database/db_scheme.cpp \
    log_manager.cpp \
    log_saver/log_saver.cpp \
    log_saver/log_saver_controller.cpp \
    worker.cpp \
    server_protocol_base.cpp \
#    old/server_protocol_2_0.cpp \
#    old/log_synchronizer_2_0.cpp \
#    old/server_protocol_2_1.cpp \
#    old/log_synchronizer_2_1.cpp \
#    old/structure_synchronizer_2_1.cpp \
#    old/structure_synchronizer_base_2_1.cpp \
    server_protocol.cpp \
    log_synchronizer.cpp \
    structure_synchronizer.cpp \
    database/db_thread_manager.cpp \
    base_synchronizer.cpp \
    command_line_parser.cpp \
    dbus_object.cpp

HEADERS += \
    database/db_scheme.h \
    log_manager.h \
    log_saver/log_saver.h \
    log_saver/log_saver_base.h \
    log_saver/log_saver_cache.h \
    log_saver/log_saver_controller.h \
    log_saver/log_saver_data.h \
    log_saver/log_saver_def.h \
    log_saver/log_saver_helper.h \
    worker.h \
    server.h \
    server_protocol_base.h \
#    old/server_protocol_2_0.h \
#    old/log_synchronizer_2_0.h \
#    old/server_protocol_2_1.h \
#    old/log_synchronizer_2_1.h \
#    old/structure_synchronizer_2_1.h \
#    old/structure_synchronizer_base_2_1.h \
    server_protocol.h \
    log_synchronizer.h \
    structure_synchronizer.h \
    database/db_thread_manager.h \
    base_synchronizer.h \
    command_line_parser.h \
    dbus_object.h

unix {
    target.path = /opt/das
    INSTALLS += target
}
