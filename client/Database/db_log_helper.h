#ifndef DAS_DB_LOG_HELPER_H
#define DAS_DB_LOG_HELPER_H

#include <functional>

#include <QPair>

#include <Helpz/db_thread.h>
#include <Helpz/db_table.h>

#include <Das/log/log_pack.h>

namespace Das {
namespace DB {

class Log_Helper
{
public:
    Log_Helper(Helpz::DB::Thread* db_thread);

    void log_value_data(const QPair<uint32_t, uint32_t>& range, std::function<void (const QVector<uint32_t>&, const QVector<Log_Value_Item>&)> callback);
    void log_event_data(const QPair<uint32_t, uint32_t>& range, std::function<void (const QVector<uint32_t>&, const QVector<Log_Event_Item>&)> callback);

private:
    template<typename T>
    void log_data(const QPair<uint32_t, uint32_t>& range, std::function<void (const QVector<uint32_t>&, const QVector<T>&)>& callback);

    Helpz::DB::Thread* db_;
};

} // namespace DB
} // namespace Das

#endif // DAS_DB_LOG_HELPER_H
