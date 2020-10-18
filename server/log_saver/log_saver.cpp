
#include <QFile>

#include <Helpz/db_base.h>

#include "log_saver.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace std;
using namespace Helpz::DB;

template<typename T> void after_process_pack(Base& /*db*/, uint32_t /*scheme_id*/, const QVector<T>& /*pack*/) {}
template<> void after_process_pack<Log_Param_Item>(Base& db, uint32_t scheme_id, const QVector<Log_Param_Item>& pack)
{
    // Save current param value

    Table table = db_table<DIG_Param_Value>();

    using T = DIG_Param_Value;
    QStringList field_names{
        table.field_names().at(T::COL_timestamp_msecs),
        table.field_names().at(T::COL_user_id),
        table.field_names().at(T::COL_value)
    };

    table.field_names() = std::move(field_names);

    const QString where = "scheme_id = " + QString::number(scheme_id) + " AND group_param_id = ";
    QVariantList values{QVariant(), QVariant(), QVariant()};

    for(const Log_Param_Item& item: pack)
    {
        values.front() = item.timestamp_msecs();
        values[1] = item.user_id();
        values.back() = item.value();

        const QSqlQuery q = db.update(table, values, where + QString::number(item.group_param_id()));
        if (!q.isActive() || q.numRowsAffected() <= 0)
        {
            Table ins_table = db_table<DIG_Param_Value>();
            ins_table.field_names().removeFirst(); // remove id

            const QVariantList ins_values{ item.timestamp_msecs(), item.user_id(), item.group_param_id(),
                                           item.value(), scheme_id };
            if (!db.insert(ins_table, ins_values))
            {
                // TODO: do something
            }
        }
    }
}

template<> void after_process_pack<Log_Mode_Item>(Base& db, uint32_t scheme_id, const QVector<Log_Mode_Item>& pack)
{
    Table table = db_table<DIG_Mode>();

    using T = DIG_Mode;
    QStringList field_names{
        table.field_names().at(T::COL_timestamp_msecs),
        table.field_names().at(T::COL_user_id),
        table.field_names().at(T::COL_mode_id)
    };

    table.field_names() = std::move(field_names);

    const QString where = "scheme_id = " + QString::number(scheme_id) + " AND group_id = ";
    QVariantList values{QVariant(), QVariant(), QVariant()};

    for(const Log_Mode_Item& mode: pack)
    {
        values.front() = mode.timestamp_msecs();
        values[1] = mode.user_id();
        values.back() = mode.mode_id();

        const QSqlQuery q = db.update(table, values, where + QString::number(mode.group_id()));
        if (!q.isActive() || q.numRowsAffected() <= 0)
        {
            Table ins_table = db_table<DIG_Mode>();
            ins_table.field_names().removeFirst(); // remove id

            QVariantList ins_values{ mode.timestamp_msecs(), mode.user_id(), mode.group_id(), mode.mode_id(), scheme_id };
            if (!db.insert(ins_table, ins_values))
            {
                // TODO: do something
            }
        }
    }
}

void save_dump_to_file(size_t type_code, const QVariantList &data)
{
    const QString file_name = QString("fail_log_%1.dat").arg(type_code);
    QFile file(file_name);

    static mutex m;
    lock_guard lock(m);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QDataStream ds(&file);
        ds << data;
    }
    else
        qCritical() << "Log finally lost. Can't open dump file: " << file_name << file.errorString();
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das
