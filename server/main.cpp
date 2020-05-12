#include <QCoreApplication>

#include <Helpz/service.h>

#include <Das/daslib_global.h>

#include "worker.h"

namespace Das {
#define STR(x) #x
    QString getVersionString() { return STR(VER_MJ) "." STR(VER_MN) "." STR(VER_B); }
}

int main(int argc, char *argv[])
{
    SET_DAS_META("DasServer");

    using Service = Helpz::Service::Impl<Das::Server::Worker>;
    return Service::instance(argc, argv).exec();
}
