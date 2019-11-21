TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS += helpz lib plus emulator

lib.depends = helpz
plus.subdir = lib/plus
plus.depends = lib

emulator.subdir = client/emulator
emulator.depends = lib plus
