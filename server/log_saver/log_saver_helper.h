#ifndef DAS_SERVER_LOG_SAVER_HELPER_H
#define DAS_SERVER_LOG_SAVER_HELPER_H

#include <Das/log/log_pack.h>

#include "log_saver_def.h"

namespace Das {
namespace Server {
namespace Log_Saver {

template<typename T> struct Type_Base { using Type = void; };
template<typename T> struct Cache_Type { using Type = void; using Is_Comparable = false_type; };

template<typename T>
struct Cache_Helper
{
    using B = typename Type_Base<T>::Type;
    static bool is_item_less(const B& item1, const B& item2) { return compare(item1, item2) < 0; }
    static bool is_item_equal(const B& item1, const B& item2) { return compare(item1, item2) == 0; }
    static int compare(const B&, const B&) { return 1; }

    static vector<uint32_t> get_compared_fields() { return {}; }
    static vector<uint32_t> get_values_fields() { return {}; }
};

#define DECL_LOG_SAVER_HELPER_VALUE_TYPE(X, Y, Z) \
    template<> struct Type_Base<Y> { using Type = DB::Z; }; \
    template<> struct Cache_Type<X> { using Type = Y; using Is_Comparable = true_type; };

// -----  Cache_Helper<T>::compare and Helper<T>::get_compared_fields
#define DECL_LOG_SAVER_HELPER_COMPARE(X, Y) \
    template<> inline int Cache_Helper<X>::compare(const typename Type_Base<X>::Type& item1,   \
                                                   const typename Type_Base<X>::Type& item2) { \
        return item1.Y() < item2.Y() ? -1 : \
               item1.Y() > item2.Y() ?  1 : 0; \
    } \
    template<> inline vector<uint32_t> Cache_Helper<X>::get_compared_fields() { \
        return { X::COL_timestamp_msecs, X::COL_##Y, X::COL_scheme_id };  \
    }

#define DECL_LOG_SAVER_HELPER(X, Y, Z, K) \
    DECL_LOG_SAVER_HELPER_VALUE_TYPE(X, Y, Z) \
    DECL_LOG_SAVER_HELPER_COMPARE(Y, K)

DECL_LOG_SAVER_HELPER(Log_Param_Item,  DIG_Param_Value,   DIG_Param_Value_Base2,  group_param_id)
DECL_LOG_SAVER_HELPER(Log_Mode_Item,   DIG_Mode,          DIG_Mode_Base,          group_id)
DECL_LOG_SAVER_HELPER(Log_Value_Item,  Device_Item_Value, Device_Item_Value_Base, item_id)

DECL_LOG_SAVER_HELPER_VALUE_TYPE(Log_Status_Item, DIG_Status,        DIG_Status_Base)

template<>
inline int Cache_Helper<DIG_Status>::compare(const DB::DIG_Status_Base& item1, const DB::DIG_Status_Base& item2)
{
    return item1.group_id() < item2.group_id() ? -1 :
           item1.group_id() > item2.group_id() ?  1 :
           item1.status_id() < item2.status_id() ? -1 :
           item1.status_id() > item2.status_id() ?  1 : 0;
}

template<>
inline vector<uint32_t> Cache_Helper<DIG_Status>::get_compared_fields()
{
    using T = DIG_Status;
    return { T::COL_timestamp_msecs, T::COL_group_id, T::COL_status_id, T::COL_scheme_id };
}

// -----  Cache_Helper<T>::get_values_fields
#define DECL_LOG_SAVER_HELPER_VALUES_FIELDS(X, Y) \
    template<> inline vector<uint32_t> Cache_Helper<X>::get_values_fields() { \
        return { X::COL_timestamp_msecs, X::COL_user_id, X::COL_##Y };  \
    }

DECL_LOG_SAVER_HELPER_VALUES_FIELDS(DIG_Param_Value, value)
DECL_LOG_SAVER_HELPER_VALUES_FIELDS(DIG_Mode,        mode_id)
DECL_LOG_SAVER_HELPER_VALUES_FIELDS(DIG_Status,      args)

template<>
inline vector<uint32_t> Cache_Helper<Device_Item_Value>::get_values_fields()
{
    using T = Device_Item_Value;
    return { T::COL_timestamp_msecs, T::COL_user_id, T::COL_raw_value, T::COL_value };
}


} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_HELPER_H
