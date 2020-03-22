#include <QDateTime>

#include <Helpz/db_builder.h>

#include <Das/log/log_type.h>

#include "db_log_helper.h"

namespace Das {
namespace DB {

Log_Helper::Log_Helper(Helpz::DB::Thread* db_thread) :
    db_(db_thread)
{
}

void Log_Helper::log_value_data(const QPair<uint32_t, uint32_t>& range, std::function<void(const QVector<uint32_t>&, const QVector<Log_Value_Item>&)> callback)
{
    log_data(range, callback);
}

void Log_Helper::log_event_data(const QPair<uint32_t, uint32_t>& range, std::function<void(const QVector<uint32_t>&, const QVector<Log_Event_Item>&)> callback)
{
    log_data(range, callback);
}

template<typename T>
void Log_Helper::log_data(const QPair<uint32_t, uint32_t>& range, std::function<void (const QVector<uint32_t>&, const QVector<T>&)>& callback)
{
    db_->add_query([range, callback](Helpz::DB::Base* db)
    {
        QSqlQuery q = db->select(Helpz::DB::db_table<T>(), QString("WHERE id >= %1 AND id <= %2").arg(range.first).arg(range.second));
        if (q.isActive())
        {
            uint32_t id = 0, next_id = range.first;
            QVector<uint32_t> not_found;
            QVector<T> data;
            while (q.next())
            {
                id = q.value(0).toUInt();
                while (next_id < id)
                {
                    not_found.push_back(next_id++);
                }
                ++next_id;

                data.push_back(Helpz::DB::db_build<T>(q));
            }

            callback(not_found, data);
        }
    });
}

} // namespace DB
} // namespace Das
