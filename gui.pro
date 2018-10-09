TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = lib gui Service Database Network

Service.subdir = helpz/Service
Database.subdir = helpz/Database
Network.subdir = helpz/Network

lib.depends = Database Service Network
gui.depends = lib Service
