QT       += core testlib sql network websockets
QT       -= gui

TARGET = tst_libtest
CONFIG += console
CONFIG += qt warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_libtest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

DESTDIR = $${OUT_PWD}/../..
include(../../common.pri)

LIBS += -lDai -lDaiPlus -lHelpzBase -lHelpzService -lHelpzDBMeta -lHelpzDB -lHelpzNetwork -lboost_system -lboost_thread -lbotan-2
