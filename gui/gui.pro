TEMPLATE = app
TARGET = DasModern
QT += qml quick quickcontrols2 sql network websockets

CONFIG += link_pkgconfig qml_debug

disable-xcb {
    message("The disable-xcb option has been deprecated. Please use disable-desktop instead.")
    CONFIG += disable-desktop
}

SOURCES += \
    main.cpp \
    models/sectionsmodel.cpp \
    global.cpp \
    models/equipmentmodel.cpp \
    models/sensormodel.cpp \
    models/networksmodel.cpp \
    serverapicall.cpp \
    websocketclient.cpp \
    models/templatemodel.cpp \
    models/journalmodel.cpp \
    models/journalfiltermodel.cpp \
    models/journaldata.cpp

HEADERS += \
    models/sectionsmodel.h \
    global.h \
    models/equipmentmodel.h \
    models/sensormodel.h \
    models/networksmodel.h \
    serverapicall.h \
    websocketclient.h \
    models/templatemodel.h \
    models/journalmodel.h \
    models/journalfiltermodel.h \
    models/journaldata.h

#Target version
VER_MAJ = 1
VER_MIN = 1
include(../common.pri)

linux-rasp-pi2-g++ {
    CONFIG += static
    LIBS += -lwiringPi

    target.path = /opt/das
    INSTALLS += target
}

android {
    CONFIG += static
} else {
    DasLogging {
        DEFINES += DAS_WITH_LOGGING=1
        LIBS += -lHelpzBase
    }
}

WithKeyboard {
    static {
    }
    QT += svg
    QTPLUGIN += qtvirtualkeyboardplugin
    DEFINES += DAS_GUI_WITH_KEYBOARD=1
}

OTHER_FILES += main.qml

RESOURCES += \
    main.qrc

LIBS += -lDas -lHelpzDBMeta

contains(ANDROID_TARGET_ARCH,x86) {
    ANDROID_EXTRA_LIBS = \
        $$OUT_PWD/../libcrypto.so \
        $$OUT_PWD/../libssl.so
}

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_EXTRA_LIBS = \
        $$OUT_PWD/../libcrypto.so \
        $$OUT_PWD/../libssl.so
}

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/AndroidManifest.xml \
    android/res/values/libs.xml \
    android/build.gradle

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
