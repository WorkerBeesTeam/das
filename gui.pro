TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = lib gui

gui.depends = lib

DaiLogging {
    SUBDIRS += Service
    Service.subdir = helpz/Service
    gui.depends += Service

    DEFINES += DAI_WITH_LOGGING=1
    LIBS += -lHelpzService
}
