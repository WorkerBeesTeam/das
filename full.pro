TEMPLATE = subdirs

DESTDIR = $${OUT_PWD}/../

SUBDIRS = helpz Das plus dbus

Das.subdir = lib/Das
Das.depends = helpz
plus.subdir = lib/plus
plus.depends = Das
dbus.subdir = lib/dbus
dbus.depends = plus

!WithoutWebApi {
    SUBDIRS += webapi
    webapi.depends = Das plus dbus
}

!GuiOnly {
    SUBDIRS += client plugins
    plugins.subdir = client/plugins
    plugins.depends = Das plus

    client.depends = Das plus
}

CONFIG(debug, debug|release) {
    SUBDIRS += tests
    tests.depends = Das plus

    CONFIG ~= s/-O[0123s]//g
    CONFIG += -O0
}
CONFIG(release, debug|release) {
}

CONFIG(DasServer, DasServer|Raspberry) {
    SUBDIRS += server telegrambot
    server.depends = dbus
    telegrambot.depends = dbus
}

!ServerOnly {
    SUBDIRS += gui
    gui.depends = Das plus

    CONFIG(Raspberry, DasServer|Raspberry) {
#            SUBDIRS += tests/cserial
    } else { # IF Default - Desktop
        !GuiOnly {
            SUBDIRS += emulator
            emulator.subdir = client/emulator
            emulator.depends = Das plus
        }
    }
}

OTHER_FILES += \
    README.md \
    TODO \
    .gitlab-ci.yml
