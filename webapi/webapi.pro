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

LIBS += -lDas -lDasPlus -lDasDbus -lHelpzBase -lHelpzService -lHelpzNetwork -lHelpzDBMeta -lHelpzDB -lboost_system -lboost_thread

LIBS += -L/usr/local/lib -lserved

SOURCES += main.cpp \
    rest/csrf_middleware.cpp \
    rest/auth_middleware.cpp \
    rest/filter.cpp \
    rest/multipart_form_data_parser.cpp \
    rest/rest.cpp \
    rest/rest_chart.cpp \
    rest/rest_chart_data_controller.cpp \
    rest/rest_chart_param.cpp \
    rest/rest_chart_value.cpp \
    rest/rest_scheme.cpp \
    rest/rest_scheme_group.cpp \
    rest/scheme_copier.cpp \
    worker.cpp \
    websocket.cpp \
    webcommand.cpp \
    dbus_handler.cpp \
    stream/stream_server.cpp \
    stream/stream_server_thread.cpp \
    stream/stream_server_controller.cpp \
    stream/stream_node.cpp

HEADERS += \
    rest/csrf_middleware.h \
    rest/auth_middleware.h \
    rest/filter.h \
    rest/json_helper.h \
    rest/multipart_form_data_parser.h \
    rest/rest.h \
    rest/rest_chart.h \
    rest/rest_chart_data_controller.h \
    rest/rest_chart_param.h \
    rest/rest_chart_value.h \
    rest/rest_scheme.h \
    rest/rest_scheme_group.h \
    rest/scheme_copier.h \
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
