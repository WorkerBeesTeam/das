#ifndef DAS_COMMANDS_H
#define DAS_COMMANDS_H

#include <QObject>

namespace Das {
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    Q_NAMESPACE
#endif

namespace Ver { // For protocol versioning
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    Q_NAMESPACE
#endif

namespace Cmd {
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    Q_NAMESPACE
#endif

    enum Command_Type {
        AUTH = 16, // Helpz::Net::Cmd::USER_COMMAND,
        NO_AUTH,

        VERSION,
        TIME_INFO,

        RESTART,
        WRITE_TO_ITEM,
        WRITE_TO_ITEM_FILE,
        SET_MODE,           // if inform ?
        CHANGE_STATUS,      // if inform ?
        SET_DIG_PARAM_VALUES,   // if inform ?
        EXEC_SCRIPT_COMMAND,

        GET_SCHEME,
        MODIFY_SCHEME,

        LOG_DATA_REQUEST, // LogType[1] log_type
        LOG_PACK, // LogType[1] log_type

        DEVICE_ITEM_VALUES,
        GROUP_STATUSES,

        STREAM_TOGGLE,
        STREAM_PARAM,
        STREAM_DATA,

        SET_SCHEME_NAME,

        /*
            cmdCreateDevice,
            cmdSetInform,
        */

        // Enum end
        COMMAND_TYPE_COUNT
    };
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    Q_ENUM_NS(Command_Type)
#endif

} // namespace Cmd

enum Structure_Type
{
    ST_UNKNOWN,
    ST_DEVICE,
    ST_PLUGIN_TYPE,
    ST_DEVICE_ITEM,
    ST_DEVICE_ITEM_TYPE,
    ST_SAVE_TIMER,
    ST_SECTION,
    ST_DEVICE_ITEM_GROUP,
    ST_DIG_TYPE,
    ST_DIG_MODE_TYPE,
    ST_DIG_PARAM_TYPE,
    ST_DIG_STATUS_TYPE,
    ST_DIG_STATUS_CATEGORY,
    ST_DIG_PARAM,
    ST_SIGN_TYPE,
    ST_CODES,
    ST_TRANSLATION,
    ST_AUTH_GROUP,
    ST_AUTH_GROUP_PERMISSION,
    ST_USER,
    ST_USER_GROUP,

    ST_DEVICE_ITEM_VALUE,
    ST_DIG_MODE,
    ST_DIG_PARAM_VALUE,

    ST_COUNT,
    ST_ITEM_FLAG = 0x40,
    ST_HASH_FLAG = 0x80,
    ST_FLAGS = ST_ITEM_FLAG | ST_HASH_FLAG
};
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
Q_ENUM_NS(Structure_Type)
#endif

} // namespace Ver

enum WebSockCmd : uint8_t {
    WS_UNKNOWN,
    WS_AUTH,
    WS_WELCOME,

    WS_CONNECTION_STATE,
    WS_WRITE_TO_DEV_ITEM,
    WS_CHANGE_DIG_MODE,
    WS_CHANGE_DIG_PARAM_VALUES,
    WS_EXEC_SCRIPT,
    WS_RESTART,

    WS_DEV_ITEM_VALUES,
    WS_EVENT_LOG,
    WS_DIG_MODE,

    WS_STRUCT_MODIFY,

    WS_GROUP_STATUS_ADDED,
    WS_GROUP_STATUS_REMOVED,
    WS_TIME_INFO,
    WS_IP_ADDRESS,

    WS_STREAM_TOGGLE,
    WS_STREAM_DATA,

    WEB_SOCK_CMD_COUNT
};
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
Q_ENUM_NS(WebSockCmd)
#endif

} // namespace Das

#endif // DAS_COMMANDS_H
