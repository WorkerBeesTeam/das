#include <QCoreApplication>

#include <Das/daslib_global.h>
#include "worker.h"

namespace Das {
#define STR(x) #x
    QString getVersionString() { return STR(VER_MJ) "." STR(VER_MN) "." STR(VER_B); }
}

int main(int argc, char *argv[])
{
    SET_DAS_META("DasServer")
    return Das::Server::Service::instance(argc, argv).exec();
}
