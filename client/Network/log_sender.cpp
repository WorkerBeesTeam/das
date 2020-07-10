#include <Helpz/db_builder.h>

#include <Das/commands.h>

#include "worker.h"

#include "log_sender.h"

namespace Das {
namespace Ver {
namespace Client {

using namespace Helpz::DB;

Log_Sender::Log_Sender(Protocol_Base *protocol) :
    request_data_size_(200),
    protocol_(protocol)
{
}

template<typename T>
void prepare_pack(QVector<T>& /*log_data*/) {}

template<>
void prepare_pack<Log_Value_Item>(QVector<Log_Value_Item>& log_data)
{
    for (auto it = log_data.begin(); it != log_data.end(); ++it)
    {
        if (it->is_big_value())
        {
            log_data.erase(++it, log_data.end());
            break;
        }
    }
}

template<typename T>
void Log_Sender::send_log_data(const Log_Type_Wrapper& log_type)
{
    Base& db = Base::get_thread_local_instance();
    QVector<T> log_data = db_build_list<T>(db, DB::Helper::get_default_where_suffix() + " LIMIT " + QString::number(request_data_size_));
    if (!log_data.empty())
    {
        prepare_pack(log_data);

        protocol_->send(Cmd::LOG_DATA_REQUEST).answer([log_data](QIODevice& /*dev*/)
        {
            QString where = DB::Helper::get_default_suffix() + " AND "
                    + T::table_column_names().at(T::COL_timestamp_msecs) + " IN (";
            for (const T& item: log_data)
            {
                where += QString::number(item.timestamp_msecs()) + ',';
            }
            where.replace(where.length() - 1, 1, ')');

            Base& db = Base::get_thread_local_instance();
            db.del(db_table_name<T>(), where);
        })
        .timeout([this, log_type]()
        {
//            qWarning(Sync_Log) << log_type.to_string() << "log send timeout. request_data_size_:" << request_data_size_;
            request_data_size_ /= 2;
            if (request_data_size_ < 5)
            {
                request_data_size_ = 5;
            }
        }, std::chrono::seconds{5}) << log_type << log_data;
    }
}

void Log_Sender::send_data(Log_Type_Wrapper log_type, uint8_t msg_id)
{
    if (log_type.is_valid())
    {
        protocol_->send_answer(Cmd::LOG_DATA_REQUEST, msg_id) << true;

        switch (log_type.value()) {
        case LOG_VALUE:  send_log_data<Log_Value_Item>(log_type); break;
        case LOG_EVENT:  send_log_data<Log_Event_Item>(log_type); break;
        case LOG_PARAM:  send_log_data<Log_Param_Item>(log_type); break;
        case LOG_STATUS: send_log_data<Log_Status_Item>(log_type); break;
        case LOG_MODE:   send_log_data<Log_Mode_Item>(log_type); break;
        default:
            break;
        }
    }
}

} // namespace Client
} // namespace Ver
} // namespace Das
