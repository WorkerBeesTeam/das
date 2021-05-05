
#include <QFile>
#include <QDataStream>

#include <Helpz/db_base.h>

#include "log_saver_base.h"

namespace Das {
namespace Log_Saver {

using namespace std;
using namespace Helpz::DB;

Q_LOGGING_CATEGORY(Log_Saver_Log, "logSaver")

void Saver_Base::save_dump_to_file(size_t type_code, const QVariantList &data)
{
    QFile file(QString("fail_log_%1.dat").arg(type_code));

    lock_guard lock(_fail_mutex);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QDataStream ds(&file);
        ds << data;
    }
    else
        qCCritical(Log_Saver_Log) << "Log type" << type_code << "finally lost. Can't open dump file: " << file.errorString();
}

QVariantList Saver_Base::load_dump_file(size_t type_code)
{
    QVariantList data;
    QFile file(QString("fail_log_%1.dat").arg(type_code));

    lock_guard lock(_fail_mutex);
    if (file.open(QIODevice::ReadWrite))
    {
        QDataStream ds(&file);
        while (!ds.atEnd())
        {
            QVariantList pack;
            ds >> pack;
            if (ds.status() != QDataStream::Ok)
                qCWarning(Log_Saver_Log) << "Log" << type_code << "dump file read fail:"
                           << (ds.status() == QDataStream::ReadPastEnd ?
                                   "read past end" : "read corrupt data");
            else
                data += move(pack);
        }
        file.resize(0);
    }
    else
        qCCritical(Log_Saver_Log) << "Can't open log" << type_code << "dump file: " << file.errorString();
    return data;
}

} // namespace Log_Saver
} // namespace Das
