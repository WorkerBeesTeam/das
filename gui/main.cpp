#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QDebug>

#include "models/sectionsmodel.h"
#include "models/equipmentmodel.h"
#include "models/sensormodel.h"
#include "models/networksmodel.h"
#include "models/journalmodel.h"

#include "serverapicall.h"
#include <Das/log/log_pack.h>

namespace Das {
#define STR(x) #x
    QString getVersionString() { return STR(VER_MJ) "." STR(VER_MN) "." STR(VER_B) /*"1.1.100"*/; }
}

int main(int argc, char *argv[])
{
//    qDebug() << QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
#ifdef DAS_GUI_WITH_KEYBOARD
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
#endif
    qputenv("QT_LABS_CONTROLS_STYLE", "Material");

    SET_DAS_META("DasGui")

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/images/logo.png"));

    auto locale = QLocale::system();
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(locale);

    QQuickStyle::setStyle("Material");

    auto f = app.font();
    f.setPointSize(13);
    app.setFont(f);

//    QSettings settings;
//    qputenv("QT_LABS_CONTROLS_STYLE", settings.value("style").toByteArray());

    qmlRegisterType<Das::DIG_Param_Type>("Das", 1, 0, "DIG_Param_Type");

//    qmlRegisterType<Das::Gui::SectionType>("Das", 1, 0, "SectionType");
    qmlRegisterType<Das::Gui::SectionsModel>("Das", 1, 0, "SectionsModel");
    qmlRegisterType<Das::Gui::EquipmentModel>("Das", 1, 0, "EquipmentModel");
    qmlRegisterType<Das::Gui::EquipmentGroupModel>("Das", 1, 0, "EquipmentGroupModel");
    qmlRegisterType<Das::Gui::SensorModel>("Das", 1, 0, "SensorModel");
    qmlRegisterType<Das::Gui::NetworksModel>("Das", 1, 0, "NetworksModel");
    qmlRegisterType<Das::Gui::WiFiModel>("Das", 1, 0, "WiFiModel");
//    qmlRegisterType<Das::Gui::Worker>("Das", 1, 0, "Worker");
    qmlRegisterType<Das::Gui::JournalModel>("Das", 1, 0, "JournalModel");

    Das::Gui::ServerApiCall srv;

#ifdef DAS_GUI_WITH_KEYBOARD
    const bool with_keyboard = true;
#else
    const bool with_keyboard = false;
#endif

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("server", &srv);
    engine.rootContext()->setContextProperty("with_keyboard", with_keyboard);

    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    srv.init();
    return app.exec();
}
