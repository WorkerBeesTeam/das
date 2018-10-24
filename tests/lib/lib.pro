QT       += core testlib sql network websockets
QT       -= gui

TARGET = tst_libtest
CONFIG += console
CONFIG += qt warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_libtest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

INCLUDEPATH += ../../lib/

include(../../common.pri)

DESTDIR = $${DESTDIR}../
LIBS += -L$${DESTDIR} -L$${DESTDIR}/helpz

LIBS += -lDai -lDaiPlus -lHelpzService -lHelpzDB -lHelpzNetwork -lboost_system -lbotan-2
