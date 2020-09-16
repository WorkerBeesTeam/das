#ifndef DAS_SERVER_LOG_SAVER_HELPER_H
#define DAS_SERVER_LOG_SAVER_HELPER_H

#include <Das/log/log_pack.h>

#include "log_saver_def.h"

namespace Das {
namespace Server {
namespace Log_Saver {

#define DECL_LOG_SAVER_HELPER_VALUE_TYPE(X, Y) \
    template<> struct Helper_Value<X> { using Type = Y; };

template<typename T> struct Helper_Value { using Type = void; };
DECL_LOG_SAVER_HELPER_VALUE_TYPE(Log_Param_Item, DIG_Param_Value)
DECL_LOG_SAVER_HELPER_VALUE_TYPE(Log_Mode_Item, DIG_Mode)
DECL_LOG_SAVER_HELPER_VALUE_TYPE(Log_Status_Item, DIG_Status)
DECL_LOG_SAVER_HELPER_VALUE_TYPE(Log_Value_Item, Device_Item_Value)

template<typename T>
struct Helper
{
    using Value_Type = typename Helper_Value<T>::Type;
    static bool is_comparable() { return true; }

    static bool is_item_less(const T& item1, const T& item2) { return compare(item1, item2) < 0; }
    static bool is_item_equal(const T& item1, const T& item2) { return compare(item1, item2) == 0; }
    static int compare(const T&, const T&) { return 1; }

    static vector<uint32_t> get_compared_fields() { return {}; }
    static vector<uint32_t> get_values_fields() { return {}; }
};

template<>
inline bool Helper<Log_Event_Item>::is_comparable() { return false; }

// -----  Helper<T>::compare and Helper<T>::get_compared_fields
#define DECL_LOG_SAVER_HELPER_COMPARE(X, Y) \
    template<> inline int Helper<X>::compare(const X& item1, const X& item2) { \
        return item1.Y() < item2.Y() ? -1 : \
               item1.Y() > item2.Y() ?  1 : 0; \
    } \
    template<> inline vector<uint32_t> Helper<X>::get_compared_fields() { \
        return { X::COL_timestamp_msecs, X::COL_##Y, X::COL_scheme_id }; \
    }

DECL_LOG_SAVER_HELPER_COMPARE(Log_Value_Item, item_id)
DECL_LOG_SAVER_HELPER_COMPARE(Log_Param_Item, group_param_id)
DECL_LOG_SAVER_HELPER_COMPARE(Log_Mode_Item, group_id)

template<>
inline int Helper<Log_Status_Item>::compare(const Log_Status_Item& item1, const Log_Status_Item& item2)
{
    return item1.group_id() < item2.group_id() ? -1 :
           item1.group_id() > item2.group_id() ?  1 :
           item1.status_id() < item2.status_id() ? -1 :
           item1.status_id() > item2.status_id() ?  1 : 0;
}

template<>
inline vector<uint32_t> Helper<Log_Status_Item>::get_compared_fields()
{
    using T = Log_Status_Item;
    return { T::COL_timestamp_msecs, T::COL_group_id, T::COL_status_id, T::COL_scheme_id };
}

// -----  Helper<T>::get_values_fields
#define DECL_LOG_SAVER_HELPER_VALUES_FIELDS(X, Y) \
    template<> inline vector<uint32_t> Helper<X>::get_values_fields() { \
        return { X::COL_timestamp_msecs, X::COL_user_id, X::COL_##Y }; \
    }

DECL_LOG_SAVER_HELPER_VALUES_FIELDS(Log_Param_Item, value)
DECL_LOG_SAVER_HELPER_VALUES_FIELDS(Log_Mode_Item, mode_id)
DECL_LOG_SAVER_HELPER_VALUES_FIELDS(Log_Status_Item, args)

template<>
inline vector<uint32_t> Helper<Log_Value_Item>::get_values_fields()
{
    using T = Log_Value_Item;
    return { T::COL_timestamp_msecs, T::COL_user_id, T::COL_value, T::COL_raw_value };
}

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_HELPER_H
