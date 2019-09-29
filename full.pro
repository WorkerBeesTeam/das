TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = helpz lib plus dbus

lib.depends = helpz
plus.subdir = lib/plus
plus.depends = lib
dbus.subdir = lib/dbus

!GuiOnly {
    SUBDIRS += client plugins
    plugins.subdir = client/plugins
    plugins.depends = lib plus

    client.depends = lib plus
}

CONFIG(debug, debug|release) {
    SUBDIRS += tests
    tests.depends = lib plus

    CONFIG ~= s/-O[0123s]//g
    CONFIG += -O0
}
CONFIG(release, debug|release) {
}

CONFIG(DaiServer, DaiServer|Raspberry) {
    SUBDIRS += server SMTPEmail api
    server.depends = lib plus dbus
    SMTPEmail.subdir = api/SMTPEmail
    api.depends = lib plus dbus SMTPEmail
}

!ServerOnly {
    SUBDIRS += gui
    gui.depends = lib plus

    CONFIG(Raspberry, DaiServer|Raspberry) {
#            SUBDIRS += tests/cserial
    } else { # IF Default - Desktop
        !GuiOnly {
            SUBDIRS += emulator
            emulator.subdir = client/emulator
            emulator.depends = lib plus
        }
    }
}

OTHER_FILES += \
    README.md \
    TODO \
    .gitlab-ci.yml
