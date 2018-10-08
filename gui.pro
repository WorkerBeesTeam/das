TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = lib gui Service Database

Service.subdir = helpz/Service
Database.subdir = helpz/Database

lib.depends = Database
gui.depends = lib Service
