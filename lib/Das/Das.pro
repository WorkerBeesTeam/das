QT -= gui

CONFIG += target_predeps

TARGET = Das
TEMPLATE = lib

DEFINES += DAS_LIBRARY

#QMAKE_CXXFLAGS += -save-temps

android {
    CONFIG += staticlib
}

SOURCES += \
    db/device_item_group.cpp \
    db/dig_mode.cpp \
    db/dig_mode_item.cpp \
    db/dig_param.cpp \
    db/dig_param_type.cpp \
    db/dig_status.cpp \
    db/dig_status_type.cpp \
    db/dig_type.cpp \
    db/schemed_model.cpp \
    db/sign_type.cpp \
    db/user.cpp \
    db/auth_group.cpp \
    db/dig_param_value.cpp \
    db/translation.cpp \
    device_item.cpp \
    device_item_group.cpp \
    proto_scheme.cpp \
    scheme.cpp \
    section.cpp \
    timerange.cpp \
    param/paramgroup.cpp \
    checkerinterface.cpp \
    log/log_pack.cpp \
    log/log_type.cpp \
    lib.cpp \
    device.cpp \
    db/device_item_type.cpp \
    db/base_type.cpp \
    db/dig_status_category.cpp \
    db/code_item.cpp \
    db/plugin_type.cpp \
    type_managers.cpp \
    write_cache_item.cpp \
    db/device_item_value.cpp \
    db/device_item.cpp \
    db/device_extra_params.cpp \
    db/save_timer.cpp

HEADERS +=\
    db/auth_group.h \
    db/device_item_group.h \
    db/dig_mode.h \
    db/dig_mode_item.h \
    db/dig_param.h \
    db/dig_param_type.h \
    db/dig_status.h \
    db/dig_status_type.h \
    db/dig_type.h \
    db/schemed_model.h \
    db/sign_type.h \
    db/user.h \
    db/dig_param_value.h \
    db/translation.h \
    device_item.h \
    device_item_group.h \
    proto_scheme.h \
    scheme.h \
    section.h \
    timerange.h \
    param/paramgroup.h \
    checkerinterface.h \
    commands.h \
    log/log_pack.h \
    log/log_type.h \
    lib.h \
    daslib_global.h \
    device.h \
    db/device_item_type.h \
    db/base_type.h \
    db/dig_status_category.h \
    db/code_item.h \
    db/plugin_type.h \
    type_managers.h \
    write_cache_item.h \
    db/device_item_value.h \
    db/device_item.h \
    db/device_extra_params.h \
    db/save_timer.h

DESTDIR = $${OUT_PWD}/../..

#Target version
VER_MAJ = 1
VER_MIN = 5
include(../../common.pri)

LIBS += -lHelpzDBMeta
#LIBS += -lHelpzService -lHelpzDB -lHelpzNetwork

unix {
    target.path = /usr/local/lib
    INSTALLS += target
}
