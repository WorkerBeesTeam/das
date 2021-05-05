#ifndef DAS_SERVER_LOG_SAVER_BASE_H
#define DAS_SERVER_LOG_SAVER_BASE_H

#include <mutex>

#include <QVariantList>
#include <QLoggingCategory>

#include <Helpz/db_table.h>

#include "log_saver_data.h"

namespace Das {
namespace Log_Saver {

Q_DECLARE_LOGGING_CATEGORY(Log_Saver_Log)

using namespace Helpz::DB;

class Saver_Base
{
public:
    virtual ~Saver_Base() = default;

    virtual bool empty(bool cache_force = false) const = 0;
    virtual time_point get_save_time() const = 0;
    virtual shared_ptr<Data> get_data_pack(size_t max_pack_size = 100, bool cache_force = false) = 0;
    virtual void process_data_pack(shared_ptr<Data> data) = 0;

protected:
    void save_dump_to_file(size_t type_code, const QVariantList& data);
    QVariantList load_dump_file(size_t type_code);
private:
    mutex _fail_mutex;
};

} // namespace Log_Saver
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_BASE_H
