TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = lib gui Base

Base.subdir = helpz/Base
gui.depends = lib Base

DaiLogging {
    SUBDIRS += Service
    Service.subdir = helpz/Service
    gui.depends += Service
}
