#include <Helpz/dtls_server_node.h>

#include <Das/commands.h>
#include <Das/db/device_item_value.h>

#include "server.h"
#include "database/db_thread_manager.h"
#include "database/db_scheme.h"
#include "server_protocol.h"
#include "dbus_object.h"

#include "log_saver/log_saver_controller.h"
#include "log_synchronizer.h"

namespace Das {
namespace Ver {
namespace Server {

using namespace std;
using namespace Helpz::DB;

template<typename T>
using DBus_Log_Func = void (Dbus_Object::*)(const Scheme_Info&, const QVector<T>&);

template<typename T, typename K, DBus_Log_Func<K> func>
class Log_Sync final : public Log_Sync_Base
{
public:
    using Log_Sync_Base::Log_Sync_Base;
    bool process(const Scheme_Info& scheme, QIODevice& data_dev) override
    {
        QVector<T> data;
        Helpz::parse_out(Helpz::Net::Protocol::DATASTREAM_VERSION, data_dev, data);
        if (data.empty())
            return false;

        QVector<K> dbus_data;
        for (T& item: data)
        {
            item.set_scheme_id(scheme.id());
            dbus_data.push_back(K{item});
        }

        // TODO: After type K remove id, just pass data to dbus func

        QMetaObject::invokeMethod(Dbus_Object::instance(), [scheme, dbus_data](){
            (Dbus_Object::instance()->*func)(scheme, dbus_data);
        }, Qt::QueuedConnection);

        return Log_Saver::instance()->add(scheme.id(), data);
    }
};

Log_Synchronizer::Log_Synchronizer()
{
#define PUSH_LOG_SYNC(N, X, Y, F) \
    _sync.emplace(N, make_shared<Log_Sync<X, Y, &Dbus_Object::F>>());

    PUSH_LOG_SYNC(LOG_VALUE, Log_Value_Item, Log_Value_Item, device_item_values_available);
    PUSH_LOG_SYNC(LOG_EVENT, Log_Event_Item, Log_Event_Item, event_message_available);
    PUSH_LOG_SYNC(LOG_PARAM, Log_Param_Item, DIG_Param_Value, dig_param_values_changed);
    PUSH_LOG_SYNC(LOG_STATUS, Log_Status_Item, DIG_Status, status_changed);
    PUSH_LOG_SYNC(LOG_MODE, Log_Mode_Item, DIG_Mode, dig_mode_changed);
}

set<uint8_t> Log_Synchronizer::get_type_set() const
{
    set<uint8_t> log_type_set;
    for (const auto& it: _sync)
        log_type_set.insert(it.first);
    return log_type_set;
}

bool Log_Synchronizer::process(Log_Type_Wrapper type_id, QIODevice *data_dev, const Scheme_Info& scheme)
{
    auto it = _sync.find(type_id.value());
    if (it != _sync.cend())
    {
        try
        {
            return it->second->process(scheme, *data_dev);
        }
        catch (const exception& e)
        {
            qWarning(Struct_Log).noquote() << "Log_Sync::process:" << e.what() << "Scheme ID:"
                                           << scheme.id() << "Log type:" << int(type_id.value());
        }
        catch (...) {}
    }
    return false;
}

} // namespace Server
} // namespace Ver
} // namespace Das
