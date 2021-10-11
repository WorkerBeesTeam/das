#include <QSqlError>

#include <Helpz/db_builder.h>

#include <Das/commands.h>

#include "worker.h"

#include "log_sender.h"

namespace Das {
namespace Ver {
namespace Client {

using namespace Helpz::DB;

Log_Sender::Log_Sender(Protocol_Base *protocol, std::size_t request_max_data_size) :
    request_data_size_(request_max_data_size),
    request_max_data_size_(request_max_data_size),
    protocol_(protocol)
{
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

template<typename T>
void prepare_pack(QVector<T>& /*log_data*/) {}

template<>
void prepare_pack<Log_Value_Item>(QVector<Log_Value_Item>& log_data)
{
    for (auto it = log_data.begin(); it != log_data.end(); ++it)
    {
        it->set_need_to_save(true);

        // Отправлять только одно большое значение в пачке данных.
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
    Worker* w = protocol_->worker();
    std::size_t size = request_data_size_.load();
    w->db_pending()->add([log_type, size, w](Helpz::DB::Base* db) {
        QVector<T> log_data = db_build_list<T>(*db, DB::Helper::get_default_where_suffix() + " LIMIT " + QString::number(size));
        if (!log_data.empty())
        {
            prepare_pack(log_data);

            auto net = w->net_protocol();
            if (net)
            {
                auto data = std::make_shared<QVector<T>>(std::move(log_data));
                net->log_sender().send_log_data<T>(log_type, std::move(data));
            }
        }
    });
}

template<typename T>
void Log_Sender::send_log_data(const Log_Type_Wrapper &log_type, std::shared_ptr<QVector<T>> log_data)
{
    protocol_->send(Cmd::LOG_DATA_REQUEST).answer([this, log_data](QIODevice& /*dev*/)
    {
        if (request_data_size_ < request_max_data_size_)
            ++request_data_size_;
        remove_data(*log_data);
    })
    .timeout([this, log_type, log_data]()
    {
        // TODO: timeout вызывается из другого потока. Проверить чтобы Log_Sender удалялся позже чем Client_Protocol
//            qWarning(Sync_Log) << log_type.to_string() << "log send timeout. request_data_size_:" << request_data_size_;
        request_data_size_ = request_data_size_ / 2;
        if (request_data_size_ < 5)
            request_data_size_ = 5;

        send_log_data<T>(log_type, log_data);
    }, std::chrono::seconds{23}, std::chrono::seconds{10}) << log_type << *log_data;
}

template<typename T>
std::vector<uint32_t> get_compared_fields() { return {}; }

#define DECL_GET_COMPARED_FIELDS(X, Y) \
    template<> std::vector<uint32_t> get_compared_fields<X>() { return { X::COL_##Y }; }

DECL_GET_COMPARED_FIELDS(Log_Param_Item, group_param_id)
DECL_GET_COMPARED_FIELDS(Log_Mode_Item,  group_id)
DECL_GET_COMPARED_FIELDS(Log_Value_Item, item_id)

template<> std::vector<uint32_t> get_compared_fields<Log_Status_Item>() {
    using T = Log_Status_Item;
    return { T::COL_group_id, T::COL_status_id, T::COL_direction };
}

template<typename T>
void Log_Sender::remove_data(const QVector<T> &data)
{
    Base& db = Base::get_thread_local_instance();
    const QStringList field_names = T::table_column_names();
    QString sql = db.del_query(db_table_name<T>(), DB::Helper::get_default_suffix() + " AND "
            + field_names.at(T::COL_timestamp_msecs) + "=?");

    const std::vector<uint32_t> fields = get_compared_fields<T>();
    for (uint32_t field: fields)
        sql += " AND " + field_names.at(field) + "=?";

    QSqlQuery q{db.database()};
    q.prepare(sql);

    for (const T& item: data)
    {
        q.addBindValue(item.timestamp_msecs());
        for (uint32_t field: fields)
            q.addBindValue(T::value_getter(item, field));
        if (!q.exec())
            qWarning() << q.lastError().text();
    }
}

} // namespace Client
} // namespace Ver
} // namespace Das
