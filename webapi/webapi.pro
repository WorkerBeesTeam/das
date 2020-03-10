QT += core network dbus sql websockets
QT -= gui widgets

TARGET = DasWebApi
CONFIG += console
CONFIG -= app_bundle

#QMAKE_CXXFLAGS += -save-temps

TEMPLATE = app

#Target version
VER_MAJ = 1
VER_MIN = 4
include(../common.pri)

LIBS += -lDas -lDasPlus -lDasDbus -lHelpzBase -lHelpzService -lHelpzNetwork -lHelpzDBMeta -lHelpzDB -lHelpzDTLS -lbotan-2 -lboost_system -lboost_thread

LIBS += -L/usr/local/lib -lserved

SOURCES += main.cpp \
    rest/restful.cpp \
    rest/csrf_middleware.cpp \
    rest/auth_middleware.cpp \
    worker.cpp \
    websocket.cpp \
    webcommand.cpp \
    dbus_handler.cpp \
    stream/stream_server.cpp \
    stream/stream_server_thread.cpp \
    stream/stream_server_controller.cpp \
    stream/stream_node.cpp

HEADERS += \
    rest/restful.h \
    rest/csrf_middleware.h \
    rest/auth_middleware.h \
    worker.h \
    websocket.h \
    webcommand.h \
    dbus_handler.h \
    stream/stream_server.h \
    stream/stream_server_thread.h \
    stream/stream_server_controller.h \
    stream/stream_node.h

unix {
    target.path = /opt/das
    INSTALLS += target
}
