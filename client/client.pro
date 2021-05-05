QT += core network dbus sql

QT += script

TARGET = DasClient
CONFIG += console
CONFIG -= app_bundle

CONFIG -= qtquickcompiler # for Scripts/js/api.js

CONFIG (debug, debug|release) {
    CONFIG -= console
    CONFIG += app_bundle
    QT += scripttools widgets
}

TEMPLATE = app

SOURCES += main.cpp \
    Scripts/scripted_scheme.cpp \
    log_save_controller.cpp \
    worker.cpp \
    Scripts/tools/pidcontroller.cpp \
    Scripts/tools/automationhelper.cpp \
    Scripts/tools/severaltimeshelper.cpp \
    Scripts/tools/pidhelper.cpp \
    Scripts/tools/daytimehelper.cpp \
    Scripts/tools/inforegisterhelper.cpp \
    Scripts/paramgroupclass.cpp \
    Scripts/paramgroupprototype.cpp \
    Network/client_protocol.cpp \
    Network/log_sender.cpp \
    structure_synchronizer.cpp \
    worker_structure_synchronizer.cpp \
    Database/db_log_helper.cpp \
    id_timer.cpp \
    Network/client_protocol_latest.cpp \
    dbus_object.cpp \
    checker_manager.cpp

HEADERS  += \
    Network/net_proto_iface.h \
    Scripts/scripted_scheme.h \
    log_save_controller.h \
    pack_sender.h \
    worker.h \
    Scripts/tools/pidcontroller.h \
    Scripts/tools/automationhelper.h \
    Scripts/tools/severaltimeshelper.h \
    Scripts/tools/pidhelper.h \
    Scripts/tools/daytimehelper.h \
    Scripts/tools/inforegisterhelper.h \
    Scripts/paramgroupprototype.h \
    Scripts/paramgroupclass.h \
    Network/client_protocol.h \
    Network/log_sender.h \
    structure_synchronizer.h \
    worker_structure_synchronizer.h \
    Database/db_log_helper.h \
    id_timer.h \
    Network/client_protocol_latest.h \
    dbus_object.h \
    checker_manager.h

#Target version
VER_MAJ = 1
VER_MIN = 5
include(../common.pri)

unix {
    target.path = /opt/das
    INSTALLS += target
}

linux-rasp-pi2-g++|linux-opi-zero-g++|linux-rasp-pi3-g++ {
    INCLUDEPATH += $$[QT_INSTALL_HEADERS]/botan-2
}

LIBS += -lDas -lDasPlus -lDasDbus -lHelpzBase -lHelpzService -lHelpzNetwork -lHelpzDBMeta -lHelpzDB -lHelpzDTLS -lbotan-2 -lboost_system -lboost_thread -ldl

RESOURCES += \
    main.qrc
