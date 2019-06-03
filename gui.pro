TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = lib gui Base DBMeta

DBMeta.subdir = helpz/DBMeta
Base.subdir = helpz/Base
gui.depends = lib Base DBMeta

DaiLogging {
    SUBDIRS += Service
    Service.subdir = helpz/Service
    gui.depends += Service
}
