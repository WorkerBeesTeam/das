#include <dbus/dbus_interface.h>

#include "bot/scheme_item.h"

#include "connection_state.h"

namespace Das {
namespace Bot {
namespace User_Menu {

Connection_State::Connection_State()
{
}

std::string Connection_State::get_value() const
{
    uint8_t state;
    QMetaObject::invokeMethod(parameters().dbus_iface_, "get_scheme_connection_state2", Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(uint8_t, state),
        Q_ARG(uint32_t, parameters().scheme_->id()));

    return to_string(state);
}

std::string Connection_State::to_string(uint8_t state)
{
    if ((state & ~CS_FLAGS) >= CS_CONNECTED_JUST_NOW)
        return "🚀 На связи!";
    else
        return "💢 Не подключен!";
}

} // namespace User_Menu
} // namespace Bot
} // namespace Das
