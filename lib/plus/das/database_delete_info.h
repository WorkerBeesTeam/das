#ifndef DAS_DATABASE_DELETE_INFO_H
#define DAS_DATABASE_DELETE_INFO_H

#include <exception>
#include <functional>
#include <cassert>

#include <QBitArray>

#include <Helpz/db_base.h>
#include <Helpz/db_delete_row.h>

#include <Das/section.h>
#include <Das/log/log_pack.h>

#include <Das/db/dig_param.h>
#include <Das/db/dig_param_value.h>
#include <Das/db/dig_status.h>
#include <Das/db/device_item_value.h>
#include <Das/db/dig_mode.h>
#include <Das/db/user.h>
#include <Das/db/auth_group.h>
#include <Das/device.h>
#include <Das/type_managers.h>

namespace Das {
namespace DB {

using Delete_Info_List = Helpz::DB::Delete_Row_Info_List;

template<typename T>
Delete_Info_List db_delete_info(const QString&)
{
//    std::cout << "Have not db_delete_info for type " << typeid(T).name() << std::endl;
//    assert(false);
    return {};
}

template<>
inline Delete_Info_List db_delete_info< User >(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<User_Groups>(db_name), "user_id", {} },
    };
}

template<>
inline Delete_Info_List db_delete_info< Auth_Group >(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<User_Groups>(db_name), "group_id", {} },
        { Helpz::DB::db_table_name<Auth_Group_Permission>(db_name), "group_id", {} },
    };
}

template<>
inline Delete_Info_List db_delete_info< Device_Item >(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Log_Value_Item>(db_name), "item_id", {} },
        { Helpz::DB::db_table_name<Device_Item_Value>(db_name), "item_id", {} },
        { Helpz::DB::db_table_name<Device_Item>(db_name), "parent_id", {}, true }
    };
}

template<>
inline Delete_Info_List db_delete_info<Device>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device_Item>(db_name), "device_id", db_delete_info<Device_Item>(db_name) }
    };
}

template<>
inline Delete_Info_List db_delete_info<Device_Item_Type>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device_Item>(db_name), "type_id", db_delete_info<Device_Item>(db_name) }
    };
}

template<>
inline Delete_Info_List db_delete_info< Save_Timer >(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device_Item_Type>(db_name), "save_timer_id", {}, true },
    };
}

template<>
inline Delete_Info_List db_delete_info< DIG_Param >(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<DIG_Param_Value>(db_name), "group_param_id", {} },
    };
}

template<>
inline Delete_Info_List db_delete_info<DIG_Status>(const QString& /*db_name*/) { return {}; }

template<>
inline Delete_Info_List db_delete_info<Device_Item_Group>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<DIG_Mode>(db_name), "group_id", {} },
        { Helpz::DB::db_table_name<DIG_Param>(db_name), "group_id", db_delete_info<DIG_Param>(db_name) },
        { Helpz::DB::db_table_name<DIG_Status>(db_name), "group_id", {} },
        { Helpz::DB::db_table_name<Device_Item>(db_name), "group_id", {}, true },
    };
}

template<>
inline Delete_Info_List db_delete_info<Section>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device_item_Group>(db_name), "section_id", db_delete_info<Device_item_Group>(db_name) }
    };
}

template<>
inline Delete_Info_List db_delete_info<DIG_Param_Type>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<DIG_Param>(db_name), "param_id", db_delete_info<DIG_Param>(db_name) }
    };
}

template<>
inline Delete_Info_List db_delete_info<DIG_Status_Type>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<DIG_Status>(db_name), "status_id", {} }
    };
}

template<>
inline Delete_Info_List db_delete_info<DIG_Mode_Type>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device_Item_Group>(db_name), "mode_id", {}, true }
    };
}

template<>
inline Delete_Info_List db_delete_info<Sign_Type>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device_Item_Type>(db_name), "sign_id", {}, true }
    };
}

template<>
inline Delete_Info_List db_delete_info<Plugin_Type>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device>(db_name), "plugin_id", {}, true }
    };
}

template<>
inline Delete_Info_List db_delete_info<DIG_Status_Category>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<DIG_Status_Type>(db_name), "type_id", db_delete_info<DIG_Status_Type>(db_name) }
    };
}

template<>
inline Delete_Info_List db_delete_info<DIG_Type>(const QString& db_name)
{
    return {
        { Helpz::DB::db_table_name<Device_item_Group>(db_name), "type_id", db_delete_info<Device_item_Group>(db_name) },
        { Helpz::DB::db_table_name<DIG_Mode_Type>(db_name), "group_type_id", {}, true },
        { Helpz::DB::db_table_name<Device_Item_Type>(db_name), "group_type_id", db_delete_info<Device_Item_Type>(db_name) },
        { Helpz::DB::db_table_name<DIG_Param_Type>(db_name), "group_type_id", db_delete_info<DIG_Param_Type>(db_name) },
        { Helpz::DB::db_table_name<DIG_Status_Type>(db_name), "group_type_id", db_delete_info<DIG_Status_Type>(db_name) },
    };
}

// has_scheme_id
template<typename T> constexpr bool has_scheme_id() { return true; }
template<> inline constexpr bool has_scheme_id<User>() { return false; }
template<> inline constexpr bool has_scheme_id<User_Groups>() { return false; }
template<> inline constexpr bool has_scheme_id<Auth_Group>() { return false; }
template<> inline constexpr bool has_scheme_id<Auth_Group_Permission>() { return false; }

// Scheme_Table_Helper
template<typename T, auto pk_getter, int pk_num_value>
struct Scheme_Table_Helper_Impl
{
    using PK_Type = typename std::result_of<decltype(pk_getter)(T)>::type;
    static const int pk_num = pk_num_value;
    static PK_Type get_pk(const T& item) { return (item.*pk_getter)(); }
    static PK_Type get_query_pk(const QSqlQuery& q) { return q.value(pk_num).value<PK_Type>(); }
};

template<typename T>
struct Scheme_Table_Helper : Scheme_Table_Helper_Impl<T, &T::id, 0> {};

template<>
struct Scheme_Table_Helper<Device_Item_Value> :
        Scheme_Table_Helper_Impl<Device_Item_Value, &Device_Item_Value::item_id, Device_Item_Value::COL_item_id> {};
template<>
struct Scheme_Table_Helper<DIG_Param_Value> :
        Scheme_Table_Helper_Impl<DIG_Param_Value, &DIG_Param_Value::group_param_id, DIG_Param_Value::COL_group_param_id> {};
template<>
struct Scheme_Table_Helper<DIG_Mode> :
        Scheme_Table_Helper_Impl<DIG_Mode, &DIG_Mode::group_id, DIG_Mode::COL_group_id> {};

// db_delete_rows
template<typename T, typename PK_Type = typename Scheme_Table_Helper<T>::PK_Type>
bool db_delete_rows(Helpz::DB::Base& db, const QVector<PK_Type>& delete_vect, uint32_t scheme_id,
                    const QString& db_name = QString())
{
    using namespace Helpz::DB;
    Table table = db_table<T>(db_name);
    if (table.field_names().isEmpty())
        return false;

    Delete_Row_Info_List delete_array = DB::db_delete_info<T>(db_name);

    Delete_Row_Helper::FILL_WHERE_FUNC_T fill_where_func;
    if (has_scheme_id<T>())
    {
        fill_where_func = [scheme_id](const Delete_Row_Info& info, QString& where_str) -> void
        {
            Q_UNUSED(info)
            where_str += " AND scheme_id = " + QString::number(scheme_id);
        };
    }

    for (PK_Type id: delete_vect)
    {
        if (!Delete_Row_Helper(&db, QString::number(id), fill_where_func)
                .del(table.name(), delete_array, table.field_names().at(Scheme_Table_Helper<T>::pk_num)))
        {
            return false;
        }
    }
    return true;
}

} // namespace DB
} // namespace Das

#endif // DAS_DATABASE_DELETE_INFO_H
