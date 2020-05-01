
#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

#include <served/status.hpp>
#include <served/request_error.hpp>

#include <QSqlError>

#include <Helpz/db_builder.h>

#include <Das/db/plugin_type.h>
#include <Das/db/save_timer.h>
#include <Das/db/sign_type.h>
#include <Das/db/code_item.h>
#include <Das/db/translation.h>
#include <Das/db/dig_status_category.h>
#include <Das/db/dig_type.h>
#include <Das/db/dig_mode_type.h>
#include <Das/db/dig_param_type.h>
#include <Das/db/dig_status_type.h>
#include <Das/db/disabled_param.h>
#include <Das/db/disabled_status.h>
#include <Das/db/chart.h>
#include <Das/db/node.h>
#include <Das/db/device_item_type.h>
#include <Das/db/device_item_group.h>
#include <Das/device.h>
#include <Das/section.h>
#include <plus/das/structure_synchronizer_base.h>
#include <plus/das/database_delete_info.h>

#include "auth_middleware.h"
#include "rest_scheme.h"
#include "scheme_copier.h"

namespace Das {

using namespace Helpz::DB;

Scheme_Copier::Scheme_Copier(uint32_t orig_id, uint32_t dest_id, bool is_dry_run) :
    is_dry_run_(is_dry_run)
{
    copy(orig_id, dest_id);
}

void Scheme_Copier::copy(uint32_t orig_id, uint32_t dest_id)
{
    const QString suffix = "WHERE scheme_id IN (" + QString::number(orig_id) + ',' + QString::number(dest_id) + ") ORDER BY scheme_id";

    Base& db = Base::get_thread_local_instance();
    Ver::Uncheck_Foreign uncheck_foreign(&db);

    std::map<uint32_t, uint32_t> plugin_type_id_map;
    copy_table<DB::Plugin_Type>(db, suffix, orig_id, dest_id, &plugin_type_id_map);

    std::map<uint32_t, uint32_t> save_timer_id_map;
    copy_table<DB::Save_Timer>(db, suffix, orig_id, dest_id, &save_timer_id_map);

    std::map<uint32_t, uint32_t> sign_type_id_map;
    copy_table<DB::Sign_Type>(db, suffix, orig_id, dest_id, &sign_type_id_map);

    copy_table<DB::Code_Item>(db, suffix, orig_id, dest_id);
    copy_table<DB::Translation>(db, suffix, orig_id, dest_id);

    std::map<uint32_t, uint32_t> dig_status_category_id_map;
    copy_table<DB::DIG_Status_Category>(db, suffix, orig_id, dest_id, &dig_status_category_id_map);

    std::map<uint32_t, uint32_t> dig_type_id_map;
    copy_table<DB::DIG_Type>(db, suffix, orig_id, dest_id, &dig_type_id_map);

    std::map<uint32_t, uint32_t> dig_mode_type_id_map;
    copy_table<DB::DIG_Mode_Type>(db, suffix, orig_id, dest_id, &dig_mode_type_id_map, {{DB::DIG_Mode_Type::COL_group_type_id, dig_type_id_map}});

    std::map<uint32_t, uint32_t> dig_status_type_id_map;
    copy_table<DB::DIG_Status_Type>(db, suffix, orig_id, dest_id, &dig_status_type_id_map, {
                                        {DB::DIG_Status_Type::COL_category_id, dig_status_category_id_map},
                                        {DB::DIG_Status_Type::COL_group_type_id, dig_type_id_map}});

    std::map<uint32_t, uint32_t> section_id_map;
    copy_table<Section>(db, suffix, orig_id, dest_id, &section_id_map);

    std::map<uint32_t, uint32_t> chart_id_map;
    copy_table<DB::Chart>(db, suffix, orig_id, dest_id, &chart_id_map);

    std::map<uint32_t, uint32_t> device_id_map;
    copy_table<Device>(db, suffix, orig_id, dest_id, &device_id_map, {{Device::COL_plugin_id, plugin_type_id_map}});

    std::map<uint32_t, uint32_t> device_item_type_id_map;
    copy_table<DB::Device_Item_Type>(db, suffix, orig_id, dest_id, &device_item_type_id_map, {
                                        {DB::Device_Item_Type::COL_group_type_id, dig_type_id_map},
                                        {DB::Device_Item_Type::COL_save_timer_id, save_timer_id_map},
                                        {DB::Device_Item_Type::COL_sign_id, sign_type_id_map}});

    std::map<uint32_t, uint32_t> dig_param_type_id_map;
    copy_table<DB::DIG_Param_Type>(db, suffix, orig_id, dest_id, &dig_param_type_id_map, {{DB::DIG_Param_Type::COL_group_type_id, dig_type_id_map}}, DB::DIG_Param_Type::COL_parent_id);

    std::map<uint32_t, uint32_t> device_item_group_id_map;
    copy_table<DB::Device_Item_Group>(db, suffix, orig_id, dest_id, &device_item_group_id_map, {
                                        {DB::Device_Item_Group::COL_type_id, dig_type_id_map},
                                        {DB::Device_Item_Group::COL_section_id, section_id_map}},
                                      DB::Device_Item_Group::COL_parent_id);

    std::map<uint32_t, uint32_t> device_item_id_map;
    copy_table<DB::Device_Item>(db, suffix, orig_id, dest_id, &device_item_id_map, {
                                        {DB::Device_Item::COL_type_id, device_item_type_id_map},
                                        {DB::Device_Item::COL_device_id, device_id_map},
                                        {DB::Device_Item::COL_group_id, device_item_group_id_map}},
                                      DB::Device_Item::COL_parent_id);

    std::map<uint32_t, uint32_t> dig_param_id_map;
    copy_table<DB::DIG_Param>(db, suffix, orig_id, dest_id, &dig_param_id_map, {
                                        {DB::DIG_Param::COL_group_id, device_item_group_id_map},
                                        {DB::DIG_Param::COL_param_id, dig_param_type_id_map}},
                                      DB::DIG_Param::COL_parent_id);

    copy_table<DB::Chart_Item>(db, suffix, orig_id, dest_id, nullptr, {
                                        {DB::Chart_Item::COL_chart_id, chart_id_map},
                                        {DB::Chart_Item::COL_param_id, dig_param_id_map},
                                        {DB::Chart_Item::COL_item_id, device_item_id_map}});

    copy_table<DB::Disabled_Param>(db, suffix, orig_id, dest_id, nullptr, {{DB::Disabled_Param::COL_param_id, dig_param_type_id_map}});
    copy_table<DB::Disabled_Status>(db, suffix, orig_id, dest_id, nullptr, {{DB::Disabled_Status::COL_status_id, dig_status_type_id_map}});

    std::map<uint32_t, uint32_t> node_id_map;
    copy_table<DB::Node>(db, suffix, orig_id, dest_id, &node_id_map, {}, DB::Node::COL_parent_id);
}

template<typename T>
void Scheme_Copier::copy_table(Base& db, const QString &suffix, uint32_t orig_id, uint32_t dest_id,
                               std::map<uint32_t, uint32_t> *id_map, const std::map<int, std::map<uint32_t, uint32_t>>& relations_id_map, int self_column_index)
{
    QString suffix_plus = suffix;
    if (self_column_index != -1)
    {
        // ORDER BY scheme_id,parent_id
        suffix_plus += ',';
        suffix_plus += T::table_column_names().at(self_column_index);
    }

    std::vector<T> orig_vect = db_build_list<T, std::vector>(db, suffix_plus), dest_vect, upd_vect;
    fill_items(orig_vect, orig_id, dest_vect, dest_id);

    fill_relations_ids(orig_vect, relations_id_map);

    // When loop start it move back
    std::vector<T> skipped_vect = std::move(orig_vect);

    std::size_t before_skiped_count;
    do
    {
        // This is necessary to check that skipped_vect is decreasing.
        before_skiped_count = skipped_vect.size();

        // Try to insert skipped items again
        std::vector<T> tmp_vect;
        orig_vect = std::move(skipped_vect);
        skipped_vect = std::move(tmp_vect);

        fill_vects(dest_id, skipped_vect, orig_vect, dest_vect, upd_vect, id_map, self_column_index);
        proc_vects(db, orig_vect, dest_vect, upd_vect, id_map);

        // In next iteration we don't need to delete or update items
        dest_vect.clear();
        upd_vect.clear();
    }
    while (!skipped_vect.empty() && skipped_vect.size() < before_skiped_count);
}

template<typename T>
void Scheme_Copier::fill_relations_ids(std::vector<T> &orig, const std::map<int, std::map<uint32_t, uint32_t> > &relations_id_map)
{
    std::map<int, std::map<uint32_t, uint32_t>>::const_iterator id_map_it;
    std::map<uint32_t, uint32_t>::const_iterator id_it;
    uint32_t orig_id;
    for (T& item: orig)
    {
        for (id_map_it = relations_id_map.cbegin(); id_map_it != relations_id_map.cend(); ++id_map_it)
        {
            orig_id = T::value_getter(item, id_map_it->first).toUInt();
            if (orig_id == 0)
                continue;

            id_it = id_map_it->second.find(orig_id);
            if (id_it == id_map_it->second.cend())
            {
                qCritical() << "Scheme_Copier: Relations map: Find id failed" << orig_id << "scheme_id:" << item.scheme_id() << T::value_getter(item, 1).toString();
            }
            else
                T::value_setter(item, id_map_it->first, id_it->second);
        }
    }
}

template<typename T>
void Scheme_Copier::fill_items(std::vector<T> &orig, uint32_t orig_id, std::vector<T> &dest, uint32_t dest_id)
{
    typename std::vector<T>::iterator orig_begin =
            std::find_if(orig.begin(), orig.end(), [orig_id](const T& item) { return item.scheme_id() == orig_id; });
    typename std::vector<T>::iterator dest_begin =
            std::find_if(orig.begin(), orig.end(), [dest_id](const T& item) { return item.scheme_id() == dest_id; });
    typename std::vector<T>::iterator orig_end, dest_end;

    if (orig_begin > dest_begin)
    {
        orig_end = orig.end();
        dest_end = orig_begin;
    }
    else
    {
        dest_end = orig.end();
        orig_end = dest_begin;
    }

//    auto it = std::next(v1.begin(), 17);
    std::move(dest_begin, dest_end, std::back_inserter(dest));
    orig.erase(dest_begin, dest_end);
}

template<typename T>
bool compare_item_key_index(const T& t1, const T& t2, int i)
{
    return T::value_getter(t1, i) == T::value_getter(t2, i);
}

template<typename T, int... key_index>
struct Scheme_Copy_Trails_Impl
{
    static bool is_compare_skip_field_pos(int pos)
    {
        return ((pos == key_index) || ...);
    }
    static bool compare_item_key(const T& t1, const T& t2)
    {
        return (compare_item_key_index<T>(t1, t2, key_index) && ...);
    }
};

template<typename T>
struct Scheme_Copy_Trails : Scheme_Copy_Trails_Impl<T, T::COL_name> {};

#define SCT_WRAP_T(x, a) x::COL_##a
#define SCT_WRAP_T_1(x, a) SCT_WRAP_T(x, a)
#define SCT_WRAP_T_2(x, a, ...) SCT_WRAP_T(x, a), SCT_WRAP_T_1(x, __VA_ARGS__)
#define SCT_WRAP_T_3(x, a, ...) SCT_WRAP_T(x, a), SCT_WRAP_T_2(x, __VA_ARGS__)

#define SCT_WRAP_PASTER(x, N, ...)      SCT_WRAP_T_##N (x, __VA_ARGS__)
#define SCT_WRAP_EVALUATOR(x, N, ...)   SCT_WRAP_PASTER(x, N, __VA_ARGS__)
#define SCT_WRAP(x, N, ...)             SCT_WRAP_EVALUATOR(x, N, __VA_ARGS__)

#define SCT_PP_NARG(...) \
        SCT_PP_NARG_(__VA_ARGS__, SCT_PP_RSEQ_N())
#define SCT_PP_NARG_(...) \
        SCT_PP_ARG_N(__VA_ARGS__)
#define SCT_PP_ARG_N(_1, _2, _3, N, ...) N
#define SCT_PP_RSEQ_N() 3,2,1,0

#define DECL_SCHEME_COPY_TRAILS(x, ...) \
    template<> \
    struct Scheme_Copy_Trails<x> : Scheme_Copy_Trails_Impl<x, SCT_WRAP(x, SCT_PP_NARG(__VA_ARGS__), __VA_ARGS__)> {};


DECL_SCHEME_COPY_TRAILS(DB::Save_Timer,         interval)
DECL_SCHEME_COPY_TRAILS(DB::Translation,        lang)
DECL_SCHEME_COPY_TRAILS(DB::DIG_Param,          group_id, param_id)
DECL_SCHEME_COPY_TRAILS(DB::Device_Item_Group,  section_id, type_id)
DECL_SCHEME_COPY_TRAILS(DB::Device_Item,        device_id, group_id, type_id)
DECL_SCHEME_COPY_TRAILS(DB::Chart_Item,         chart_id, item_id, param_id)
DECL_SCHEME_COPY_TRAILS(DB::Disabled_Param,     group_id, param_id)
DECL_SCHEME_COPY_TRAILS(DB::Disabled_Status,    group_id, status_id)
DECL_SCHEME_COPY_TRAILS(DB::Node,               name, type_id, parent_id)

template<typename T>
void Scheme_Copier::fill_vects(uint32_t dest_id, std::vector<T>& skipped_vect, std::vector<T>& insert_vect, std::vector<T>& delete_vect, std::vector<T>& update_vect, std::map<uint32_t, uint32_t>* id_map, int self_column_index)
{
    std::vector<T>& orig_vect = insert_vect;
    std::vector<T>& dest_vect = delete_vect;
    typename std::vector<T>::iterator dest_it;

    uint32_t orig_self_id;
    std::map<uint32_t, uint32_t>::const_iterator id_it;

    bool is_full_identical;

    for (auto orig_it = orig_vect.begin(); orig_it != orig_vect.end(); )
    {
        if (id_map && self_column_index != -1)
        {
            orig_self_id = T::value_getter(*orig_it, self_column_index).toUInt();
            if (orig_self_id)
            {
                id_it = id_map->find(orig_self_id);
                if (id_it == id_map->cend())
                {
                    skipped_vect.push_back(std::move(*orig_it));
                    orig_it = orig_vect.erase(orig_it);
                    continue;
                }
                else
                {
                    T::value_setter(*orig_it, self_column_index, id_it->second);
                }
            }
        }

        for (dest_it = dest_vect.begin(); dest_it != dest_vect.end(); ++dest_it)
        {
            if (Scheme_Copy_Trails<T>::compare_item_key(*orig_it, *dest_it))
            {
                is_full_identical = true;
                for (int i = T::COL_id + 1; i < T::COL_scheme_id; ++i)
                {
                    if (Scheme_Copy_Trails<T>::is_compare_skip_field_pos(i))
                        continue;

                    if (!compare_item_key_index<T>(*orig_it, *dest_it, i))
                    {
                        is_full_identical = false;
                        break;
                    }
                }

                break;
            }
        }

        if (dest_it != dest_vect.end())
        {
            if (id_map)
                id_map->emplace(orig_it->id(), dest_it->id());

            if (!is_full_identical)
            {
                orig_it->set_id(dest_it->id());
                orig_it->set_scheme_id(dest_it->scheme_id());
                update_vect.push_back(std::move(*orig_it));
            }

            dest_vect.erase(dest_it);
            orig_it = orig_vect.erase(orig_it);
        }
        else
            ++orig_it;
    }

    for (auto insert_it = insert_vect.begin(); insert_it != insert_vect.end(); )
    {
        insert_it->set_scheme_id(dest_id);

        if (delete_vect.empty())
        {
            ++insert_it;
            continue;
        }

        dest_it = delete_vect.begin();
        if (id_map)
            id_map->emplace(insert_it->id(), dest_it->id());

        insert_it->set_id(dest_it->id());
        update_vect.push_back(std::move(*insert_it));

        delete_vect.erase(dest_it);
        insert_it = insert_vect.erase(insert_it);
    }
}

template<typename T>
void Scheme_Copier::proc_vects(Base& db, const std::vector<T>& insert_vect, const std::vector<T>& delete_vect, const std::vector<T>& update_vect, std::map<uint32_t, uint32_t>* id_map)
{
    Table table = db_table<T>();

    auto res_it = result_.find(table.name().toStdString());
    Item& scheme_item =
            (res_it == result_.cend()) ?
                result_.emplace(table.name().toStdString(), Item{0, 0, 0, 0, 0, 0}).first->second : res_it->second;

    if (!delete_vect.empty())
    {
        Delete_Row_Info_List delete_array = DB::db_delete_info<T>(QString());
        const int pk_index = DB::Scheme_Table_Helper<T>::pk_num;

        const Delete_Row_Info row_info{table, pk_index, delete_array, false, pk_index};

        for (const T& item: delete_vect)
        {
            if (is_dry_run_ || Delete_Row_Helper(&db, QString::number(item.id()))
                    .del(row_info))
            {
                ++scheme_item.counter_[Item::SCI_DELETED];
                qDebug() << "Scheme_Copier: Delete:" << item.id() << "scheme_id:" << item.scheme_id() << "table" << table.name() << T::value_getter(item, 1).toString();
            }
            else
            {
                ++scheme_item.counter_[Item::SCI_DELETE_ERROR];
                qDebug() << "Scheme_Copier: Delete error:" << item.id() << "scheme_id:" << item.scheme_id() << "table" << table.name() << T::value_getter(item, 1).toString();
            }
        }
    }

    int i;

    if (!update_vect.empty())
    {
        QSqlQuery query(db.database());
        query.prepare(db.update_query(table, T::COL_COUNT + 1, "id = ?"));

        for (const T& item: update_vect)
        {
            for (i = 0; i < T::COL_COUNT; ++i)
                query.bindValue(i, T::value_getter(item, i));
            query.bindValue(i, item.id());

            if (is_dry_run_ || query.exec())
            {
                ++scheme_item.counter_[Item::SCI_UPDATED];
                qDebug() << "Scheme_Copier: Update:" << item.id() << "scheme_id:" << item.scheme_id() << "table" << table.name() << T::value_getter(item, 1).toString();
            }
            else
            {
                ++scheme_item.counter_[Item::SCI_UPDATE_ERROR];
                qWarning() << "Scheme_Copier: Update error:" << item.id() << "scheme_id:" << item.scheme_id() << "table" << table.name() << T::value_getter(item, 1).toString()
                           << "Error:" << query.lastError().text();
            }
        }
    }

    if (!insert_vect.empty())
    {
        table.field_names().removeFirst();

        QSqlQuery query(db.database());
        query.prepare(db.insert_query(table, T::COL_COUNT - 1));

        for (const T& item: insert_vect)
        {
            for (i = 1; i < T::COL_COUNT; ++i)
                query.bindValue(i - 1, T::value_getter(item, i));

            if (is_dry_run_ || query.exec())
            {
                ++scheme_item.counter_[Item::SCI_INSERTED];
                qDebug() << "Scheme_Copier: Insert:" << item.id() << "scheme_id:" << item.scheme_id() << "table" << table.name() << T::value_getter(item, 1).toString();

                if (id_map)
                {
                    const uint32_t new_id = is_dry_run_ ? random() : query.lastInsertId().toUInt();
                    id_map->emplace(item.id(), new_id);
                }
            }
            else
            {
                ++scheme_item.counter_[Item::SCI_INSERT_ERROR];
                qDebug() << "Scheme_Copier: Insert error:" << item.id() << "scheme_id:" << item.scheme_id() << "table" << table.name() << T::value_getter(item, 1).toString()
                         << "Error:" << query.lastError().text();
            }
        }
    }
}

} // namespace Das
