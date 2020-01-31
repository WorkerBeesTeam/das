#include "worker.h"
//#include "version.h"

namespace Das {
#define STR(x) #x
    QString getVersionString() { return STR(VER_MJ) "." STR(VER_MN) "." STR(VER_B); }
}

int main(int argc, char *argv[])
{
    SET_DAS_META("DasClient")

    QLocale loc = QLocale::system(); // current locale
    loc.setNumberOptions(QLocale::c().numberOptions()); // borrow number options from the "C" locale
    QLocale::setDefault(loc); // set as default

    return Das::Service::instance(argc, argv).exec();
}
