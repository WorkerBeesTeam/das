TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = lib helpz client plugins

lib.depends = helpz
plugins.subdir = client/plugins
plugins.depends = lib

client.depends = lib

CONFIG(debug, debug|release) {
    SUBDIRS += tests
    tests.depends = lib

    CONFIG ~= s/-O[0123s]//g
    CONFIG += -O0
}
CONFIG(release, debug|release) {
}

#CONFIG(DaiServer, DaiServer|Raspberry) {
#    SUBDIRS += server
#    server.subdir = server_old
#    server.depends = lib
#}

!ServerOnly {
    SUBDIRS += gui
    gui.depends = lib client

    CONFIG(Raspberry, DaiServer|Raspberry) {
#            SUBDIRS += tests/cserial
    } else { # IF Default - Desktop
        SUBDIRS += emulator
        emulator.subdir = client/emulator
        emulator.depends = lib
    }
}

OTHER_FILES += \
    README.md \
    TODO \
    .gitlab-ci.yml
