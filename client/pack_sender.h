#ifndef DAS_PACK_SENDER_H
#define DAS_PACK_SENDER_H

#include <future>
#include <chrono>
#include <memory>

#include <QTimer>

#include <Helpz/db_base.h>

#include <Das/log/log_type.h>
#include <Das/log/log_pack.h>
#include <Das/commands.h>

#include "Network/net_proto_iface.h"

namespace Das {

template<typename T>
struct Pack_Send_Item
{
    Pack_Send_Item(QVector<T>&& data) : _data{std::move(data)} {}
    std::promise<bool> _promise;
    QVector<T> _data;
};

template<typename T> bool can_log_item_save(const T& /*item*/) { return true; }
template<> inline bool can_log_item_save<Log_Value_Item>(const Log_Value_Item& item) { return item.need_to_save(); }

template<typename T>
class Pack_Sender
{
public:
    Pack_Sender(Log_Type type, Net_Protocol_Interface* net_ctrl) :
        _type(type), _net_ctrl(net_ctrl)
    {
        _timer.setSingleShot(true);
    }

    ~Pack_Sender()
    {
        if (_future.valid())
            _future.get();
        save_to_db(_data);
    }

    bool send()
    {
        using namespace std::chrono_literals;

        for (T& item: _new_data)
            _data.push_back(std::move(item));
        _new_data.clear();

        if (_data.empty()
         || (_future.valid() && _future.wait_for(0s) != std::future_status::ready))
        {
            _timer.start(100);
            return false;
        }

        for (T& item: _data)
        {
            if (item.timestamp_msecs() == 0)
            {
                qWarning() << "Attempt to save log with zero timestamp";
                item.set_current_timestamp();
            }
        }

        auto proto = _net_ctrl->net_protocol();
        if (proto)
        {
            auto pack = std::make_shared<Pack_Send_Item<T>>(std::move(_data));
            _future = pack->_promise.get_future();

            proto->send(Ver::Cmd::LOG_PACK)
                .answer([](QIODevice&){}) // for finaly is_ok == false when timeout or just clear
                .timeout(nullptr, 11s, 5s) // TODO: Make timeouts configurable
                .finally([this, pack](bool is_ok) {
                    if (!is_ok) // When timeout or just delete net message.
                        save_to_db(pack->_data);
                    pack->_promise.set_value(is_ok);
                })
                << _type << pack->_data;
        }
        else
            save_to_db(_data);

        _data.clear();
        return true;
    }

    QTimer& timer() { return _timer; }
    QVector<T>& data() { return _new_data; }

private:

    bool save_to_db(const QVector<T>& data)
    {
        if (data.empty())
            return true;

        QVariantList values;
        for (const T& item: data)
            if (can_log_item_save<T>(item))
                values += T::to_variantlist(item);

        if (values.empty())
            return true;

        using namespace Helpz::DB;
        const Table table = db_table<T>();
        const QString sql = "INSERT INTO " + table.name() + '(' + table.field_names().join(',') + ") VALUES" +
                Base::get_q_array(table.field_names().size(), values.size() / table.field_names().size());

        Base& db = Base::get_thread_local_instance();
        return db.exec(sql, values).isActive();
    }

    Log_Type_Wrapper _type;
    QVector<T> _new_data, _data;
    std::future<bool> _future;

    Net_Protocol_Interface* _net_ctrl;
    QTimer _timer;
};

} // namespace Das

#endif // DAS_PACK_SENDER_H
