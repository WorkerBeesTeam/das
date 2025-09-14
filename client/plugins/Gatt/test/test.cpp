#include "gatt_test.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>

#include <iostream>

using namespace Das::Gatt;

void my_message_output(QtMsgType t, const QMessageLogContext &ctx,
                                     const QString &msg)
{
//    qt_message_output(t, ctx, msg);

    std::ostream& out = [t]() -> std::ostream& {
        switch (t) {
        case QtDebugMsg:
        case QtInfoMsg:
            return std::cout;
        default:
            return std::cerr;
        }
    }();

    switch (t) {
    case QtDebugMsg: out << "[D]"; break;
    case QtWarningMsg: out << "[W]"; break;
    case QtCriticalMsg: out << "[E]"; break;
    case QtFatalMsg: out << "[F]"; break;
    case QtInfoMsg: out << "[I]"; break;
    default:
        out << "[UNK]";
        break;
    }

    if (qstrcmp(ctx.category, "default") != 0)
        out << '[' << ctx.category << "] ";
    else
        out << ' ';

    out << msg.toStdString() << std::endl;
}

int main(int argc, char* argv[])
{
    qInstallMessageHandler(my_message_output);
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("plugin_path", "Path to gatt plugin.");
    parser.process(app);
    auto path_list = parser.positionalArguments();

    const QString plugin_path = path_list.empty() ? QLatin1String(PLUGINS_PATH) + "/libGattPlugin.so" : path_list.front();
    if (!QFile::exists(plugin_path))
    {
        qCritical() << "Plugin file doesn't exist:" << plugin_path;
        return 1;
    }

    qDebug() << "Test begining...";

    auto test = std::make_shared<Test>();
    if (!test->start(plugin_path))
    {
        qCritical() << "Start test failed";
        return 1;
    }

    return app.exec();
}
