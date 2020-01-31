#include <QApplication>

#include "mainwindow.h"
//#include "version.h"

namespace Das {
    QString getVersionString() { return "1.1.100"/*DasEmulator::Version::getVersionString()*/; }
}

int main(int argc, char *argv[])
{
    SET_DAS_META("DasEmulator")

    QApplication a(argc, argv);

    Main_Window w;
    w.show();

    return a.exec();
}
