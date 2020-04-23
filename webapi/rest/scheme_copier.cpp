
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
#include <Das/db/disabled_status.h>
#include <Das/db/chart.h>
#include <Das/db/device_item_type.h>
#include <Das/device.h>
#include <Das/section.h>
#include <plus/das/structure_synchronizer_base.h>

#include "auth_middleware.h"
#include "rest_scheme.h"
#include "scheme_copier.h"

namespace Das {

using namespace Helpz::DB;

Scheme_Copier::Scheme_Copier(uint32_t orig_id, uint32_t dest_id)
{
    copy(orig_id, dest_id);
}

void Scheme_Copier::copy(uint32_t orig_id, uint32_t dest_id)
{
    const QString suffix = "WHERE scheme_id IN (" + QString::number(orig_id) + ',' + QString::number(dest_id) + ") ORDER BY scheme_id";

    Base& db = Base::get_thread_local_instance();

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

//    const std::vector<uint8_t> structure_type_vect = Ver::Structure_Synchronizer_Base::get_main_table_types();
//    for (uint8_t structure_type: structure_type_vect)
    {
        //        ST_PLUGIN_TYPE,               1
        //        ST_SAVE_TIMER,                1
        //        ST_SIGN_TYPE,                 1
        //        ST_CODES,                     1
        //        ST_TRANSLATION,               1
        //        ST_DIG_STATUS_CATEGORY,       1
        //        ST_SECTION,                   1
        //        ST_CHART,                     1

        //        ST_DEVICE,                    1
        //        ST_DEVICE_ITEM_TYPE,          1
        //        ST_DIG_TYPE,                  1
        //        ST_DIG_MODE_TYPE,             1
        //        ST_DIG_STATUS_TYPE,           1
        //        ST_DIG_PARAM_TYPE,
        //        ST_DEVICE_ITEM,
        //        ST_DEVICE_ITEM_GROUP,
        //        ST_CHART_ITEM,
        //        ST_NODE,
        //        ST_DIG_PARAM,
        //        ST_DISABLED_PARAM,
        //        ST_DISABLED_STATUS,

        //        ST_AUTH_GROUP,
        //        ST_AUTH_GROUP_PERMISSION,
        //        ST_USER,
        //        ST_USER_GROUP
    }
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

    fill_relations_ids(orig_vect, relations_id_map, self_column_index);

    fill_vects(orig_vect, dest_vect, upd_vect, id_map);
    proc_vects(db, orig_vect, dest_vect, upd_vect, id_map);
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

template<typename T>
bool is_compare_skip_field_pos(int pos) { return pos == T::COL_name; }

template<typename T>
bool compare_item_key(const T& t1, const T& t2) { return compare_item_key_index<T>(t1, t2, T::COL_name); }

template<>
bool is_compare_skip_field_pos<DB::Save_Timer>(int pos) { return pos == DB::Save_Timer::COL_interval; }

template<>
bool compare_item_key<DB::Save_Timer>(const DB::Save_Timer& t1, const DB::Save_Timer& t2) { return compare_item_key_index(t1, t2, DB::Save_Timer::COL_interval); }

template<>
bool is_compare_skip_field_pos<DB::Translation>(int pos) { return pos == DB::Translation::COL_lang; }

template<>
bool compare_item_key<DB::Translation>(const DB::Translation& t1, const DB::Translation& t2) { return compare_item_key_index(t1, t2, DB::Translation::COL_lang); }

template<typename T>
void Scheme_Copier::fill_vects(std::vector<T>& insert_vect, std::vector<T>& delete_vect, std::vector<T>& update_vect, std::map<uint32_t, uint32_t>* id_map, int self_column_index)
{
    std::vector<T>& orig_vect = insert_vect;
    std::vector<T>& dest_vect = delete_vect;
    typename std::vector<T>::iterator dest_it;

    bool is_full_identical;

    for (auto orig_it = orig_vect.begin(); orig_it != orig_vect.end(); )
    {
        for (dest_it = dest_vect.begin(); dest_it != dest_vect.end(); ++dest_it)
        {
            if (compare_item_key(*orig_it, *dest_it))
            {
                is_full_identical = true;
                for (int i = T::COL_id + 1; i < T::COL_scheme_id; ++i)
                {
                    if (!is_compare_skip_field_pos<T>(i)
                        && T::value_getter(*orig_it, i) != T::value_getter(*dest_it, i))
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
        if (delete_vect.empty())
            continue;

        dest_it = delete_vect.begin();
        if (id_map)
            id_map->emplace(insert_it->id(), dest_it->id());

        insert_it->set_id(dest_it->id());
        insert_it->set_scheme_id(dest_it->scheme_id());
        update_vect.push_back(std::move(*insert_it));

        delete_vect.erase(dest_it);
        insert_it = insert_vect.erase(insert_it);
    }
}

template<typename T>
void Scheme_Copier::proc_vects(Base& db, const std::vector<T>& insert_vect, const std::vector<T>& delete_vect, const std::vector<T>& update_vect, std::map<uint32_t, uint32_t>* id_map)
{
    Table table = db_table<T>();

    if (!delete_vect.empty())
    {
        QString suffix = "id IN(";
        for (const T& item: delete_vect)
        {
            qDebug() << "Scheme_Copier: Delete:" << item.id() << "scheme_id:" << item.scheme_id() << T::value_getter(item, 1).toString();
            suffix += QString::number(item.id());
            suffix += ',';
        }
        suffix.replace(suffix.size() - 1, 1, ')');

        db.del(table.name(), suffix);
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

            if (!query.exec())
                qWarning() << "Scheme_Copier: Update error:" << query.lastError().text();
            else
                qDebug() << "Scheme_Copier: Update:" << item.id() << "scheme_id:" << item.scheme_id() << T::value_getter(item, 1).toString();
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
            if (!query.exec())
                qWarning() << "Scheme_Copier: Insert error:" << query.lastError().text();
            else
            {
                qDebug() << "Scheme_Copier: Insert:" << item.id() << "scheme_id:" << item.scheme_id() << T::value_getter(item, 1).toString();

                if (id_map)
                    id_map->emplace(item.id(), query.lastInsertId().toUInt());
            }
        }
    }
}

} // namespace Das
