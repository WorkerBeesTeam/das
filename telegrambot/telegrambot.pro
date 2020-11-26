QT += core network dbus sql
QT -= gui widgets

# QT += xlsx

TARGET = DasTelegramBot
CONFIG += console
CONFIG -= app_bundle

#QMAKE_CXXFLAGS += -save-temps

TEMPLATE = app

#Target version
VER_MAJ = 1
VER_MIN = 4
include(../common.pri)

LIBS += -lstdc++fs
LIBS += -L$$DESTDIR -lDas -lDasPlus -lDasDbus -lHelpzBase -lHelpzService -lHelpzDBMeta -lHelpzDB -lboost_thread

# LIBS += -L$${OUT_PWD}/SMTPEmail -lSMTPEmail
LIBS += \
    -lTgBot \
    -lboost_system \
    -lssl \
    -lcrypto \
    -lpthread \
    -lcurl \
    -L/usr/local/lib/ \
    -L/usr/local/opt/openssl/lib

SOURCES += main.cpp \
    db/tg_auth.cpp \
    db/tg_chat.cpp \
    db/tg_subscriber.cpp \
    db/tg_user.cpp \
    dbus_webapi_interface.cpp \
    worker.cpp \
    informer.cpp \
    smtp_client.cpp \
    dbus_handler.cpp \
    bot/scheme_item.cpp \
    bot/controller.cpp \
    bot/elements.cpp \
    bot/menu_item.cpp \
    bot/bot_base.cpp \
    bot/user_menu/item.cpp \
    bot/user_menu/data_item.cpp \
    bot/user_menu/connection_state.cpp \
    bot/user_menu/dig_name.cpp \
    bot/user_menu/dig_mode.cpp \
    bot/user_menu/dig_param.cpp \
    bot/user_menu/device_item.cpp \
    bot/user_menu/id_data_item.cpp \
    bot/user_menu/named_data_item.cpp \
    bot/user_menu/device_item_value_normalizer.cpp

HEADERS += \
    db/tg_auth.h \
    db/tg_chat.h \
    db/tg_subscriber.h \
    db/tg_user.h \
    dbus_webapi_interface.h \
    rest_ctrl.h \
    worker.h \
    informer.h \
    smtp_client.h \
    dbus_handler.h \
    bot/scheme_item.h \
    bot/elements.h \
    bot/controller.h \
    bot/menu_item.h \
    bot/bot_base.h \
    bot/user_menu/exprtk.hpp \
    bot/user_menu/item.h \
    bot/user_menu/data_item.h \
    bot/user_menu/connection_state.h \
    bot/user_menu/dig_name.h \
    bot/user_menu/dig_mode.h \
    bot/user_menu/dig_param.h \
    bot/user_menu/device_item.h \
    bot/user_menu/id_data_item.h \
    bot/user_menu/named_data_item.h \
    bot/user_menu/device_item_value_normalizer.h

unix {
    target.path = /opt/das
    INSTALLS += target
}
