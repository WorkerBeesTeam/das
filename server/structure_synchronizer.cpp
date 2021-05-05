#include <iostream>

#include <Helpz/db_builder.h>
#include <Helpz/dtls_server_thread.h>

#include <Das/db/translation.h>

#include <plus/das/database_delete_info.h>

#include "server.h"
#include "server_protocol.h"
#include "dbus_object.h"
#include "structure_synchronizer.h"

namespace Das {
namespace Ver {
namespace Server {

using namespace Helpz::DB;

Structure_Synchronizer::Structure_Synchronizer(Protocol_Base *protocol) :
    Base_Synchronizer(protocol),
    Structure_Synchronizer_Base(protocol->db_thread())
{
}

Structure_Synchronizer::~Structure_Synchronizer()
{
}

void Structure_Synchronizer::check(bool custom_only, bool force_clean)
{
    struct_sync_timeout_ = false;

    check_values();
    check_statuses();

    if (!custom_only && force_clean)
    {
        synchronized_.clear();
        send_scheme_request(ST_HASH_FLAG);
    }

    std::vector<uint8_t> struct_type_array = get_main_table_types();
    std::vector<uint8_t>::iterator it;
    std::set<uint8_t>::iterator sync_it;

    for (uint8_t i = 1; i < ST_COUNT; ++i)
    {
        if (struct_type_array.size())
        {
            it = std::find(struct_type_array.begin(), struct_type_array.end(), i);
            if (it != struct_type_array.end())
            {
                struct_type_array.erase(it);
//                if (custom_only || !force_clean)
                {
                    continue;
                }
            }
        }

        sync_it = synchronized_.find(i);
        if (force_clean && sync_it != synchronized_.end())
        {
            synchronized_.erase(sync_it);
            sync_it = synchronized_.end();
        }

        if (sync_it == synchronized_.end())
        {
            send_scheme_request(i | ST_HASH_FLAG);
        }
    }
}

void Structure_Synchronizer::process_modify(uint32_t user_id, uint8_t struct_type, QIODevice *data_dev)
{
    if (need_to_use_parent_table(struct_type))
        return;

    struct Server_Bad_Fix : public Bad_Fix
    {
        Structure_Synchronizer_Base* get_structure_synchronizer() override
        {
            scheme_ = std::dynamic_pointer_cast<Protocol>(node_->protocol());
            if (scheme_)
                return scheme_->structure_sync();
            return nullptr;
        }

        std::shared_ptr<Helpz::DTLS::Server_Node> node_;
        std::shared_ptr<Protocol> scheme_;
    };

    auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());
    if (!node)
    {
        qWarning(Struct_Log).noquote() << title() << "Bad node pointer process_modify";
        return;
    }
    process_modify_message(user_id, struct_type, data_dev, scheme_id(), [node]() -> std::shared_ptr<Bad_Fix>
    {
        std::shared_ptr<Server_Bad_Fix> server_bad_fix(new Server_Bad_Fix);
        server_bad_fix->node_ = node;

        return std::static_pointer_cast<Bad_Fix>(server_bad_fix);
    });
}

bool Structure_Synchronizer::need_to_use_parent_table(uint8_t struct_type) const
{
    return !protocol_->extending_scheme_ids().empty() && is_main_table(struct_type);
}

void Structure_Synchronizer::check_values()
{
    check_values_or_statuses(Cmd::DEVICE_ITEM_VALUES, &Log_Saver::Manager::set_devitem_values);
}

void Structure_Synchronizer::check_statuses()
{
    check_values_or_statuses(Cmd::GROUP_STATUSES, &Log_Saver::Manager::set_statuses);
}

template<typename T>
void Structure_Synchronizer::check_values_or_statuses(Cmd::Command_Type cmd,
                                                      void(Log_Saver::Manager::*set_cache)(QVector<T> &&, uint32_t))
{
    uint32_t scheme_id = protocol_->id();
    protocol_->send(cmd)
            .answer([set_cache, scheme_id](QIODevice& data_dev)
    {
        Helpz::apply_parse(data_dev, QDataStream::Qt_DefaultCompiledVersion, set_cache, Log_Saver::instance(), scheme_id);
    })
            .timeout([this, cmd, set_cache]()
    {
        qCWarning(Struct_Log).noquote() << title() << "Timeout to receive" << cmd;
        check_values_or_statuses<T>(cmd, set_cache);
    }, std::chrono::seconds(10), std::chrono::seconds(3));
}

Scheme_Info Structure_Synchronizer::get_scheme_info(uint8_t struct_type) const
{
    Scheme_Info scheme_info{scheme_id()};
    if (need_to_use_parent_table(struct_type))
        scheme_info.set_extending_scheme_ids(protocol()->extending_scheme_ids());
    return scheme_info;
}

void Structure_Synchronizer::set_synchronized(uint8_t struct_type)
{
    synchronized_.insert(struct_type);
    if (synchronized_.size() == ST_COUNT)
    {
        uint8_t state = protocol()->connection_state();
        protocol()->set_connection_state(CS_CONNECTED | (state & CS_FLAGS));
    }
}

void Structure_Synchronizer::send_modify_response(uint8_t /*struct_type*/, const QByteArray& buffer, uint32_t /*user_id*/)
{
    QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "structure_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *protocol_), Q_ARG(QByteArray, buffer));
}

Helpz::Net::Protocol_Sender Structure_Synchronizer::send_scheme_request(uint8_t struct_type)
{
    if (!struct_sync_timeout_)
    {
        struct_wait_set_.insert(struct_type);
    }

    Helpz::Net::Protocol_Sender sender = protocol_->send(Cmd::GET_SCHEME);

    sender.answer([this](QIODevice& data_dev)
    {
        apply_parse(data_dev, &Structure_Synchronizer::process_scheme, &data_dev);
    })
    .timeout([this, struct_type]()
    {
        struct_sync_timeout_ = true;
        struct_wait_set_.clear();

        qCWarning(Struct_Log).noquote() << title() << '[' << type_name(struct_type) << "] Timeout";

        protocol()->set_connection_state(CS_CONNECTED_SYNC_TIMEOUT | (modified() ? CS_CONNECTED_MODIFIED : 0));

        uint8_t st_type = struct_type & ~ST_FLAGS;
        send_scheme_request(st_type | ST_HASH_FLAG);
    }, std::chrono::seconds(7), std::chrono::seconds(3));

    sender << struct_type;

    qCDebug(Struct_Detail_Log).noquote() << title() << '[' << type_name(struct_type) << "] REQ GET_SCHEME";
    return sender;
}

void Structure_Synchronizer::process_scheme(uint8_t struct_type, QIODevice* data_dev)
{
//    qCDebug(Struct_Log).noquote() << title() << '[' << type_name(struct_type) << "] process_scheme";
//             << bool(struct_type & ST_HASH_FLAG) << "size" << data_dev->size();

    qCDebug(Struct_Detail_Log).noquote() << title() << '[' << type_name(struct_type) << "] GET_SCHEME";

    uint8_t flags = struct_type & ST_FLAGS;
    struct_type &= ~ST_FLAGS;

    struct_wait_set_.erase(struct_type);
    struct_wait_set_.erase(struct_type | flags);

    if (flags & ST_HASH_FLAG)
    {
        if (flags & ST_ITEM_FLAG)
        {
            apply_parse(*data_dev, &Structure_Synchronizer::process_scheme_items_hash, struct_type);
        }
        else
        {
            apply_parse(*data_dev, &Structure_Synchronizer::process_scheme_hash, struct_type);
        }
    }
    else if (!need_to_use_parent_table(struct_type))
    {
        process_scheme_data(struct_type, data_dev, (flags & ST_ITEM_FLAG) == 0);
    }
    else
    {
        qCWarning(Struct_Log).noquote() << title() << '[' << type_name(struct_type) << "] need_to_use_parent_table";
    }

    if (!struct_sync_timeout_ && struct_wait_set_.empty())
    {
        protocol()->set_connection_state(CS_CONNECTED | (modified() ? CS_CONNECTED_MODIFIED : 0));
    }
}

void Structure_Synchronizer::process_scheme_items_hash(QMap<uint32_t,uint16_t>&& client_hash_map_arg, uint8_t struct_type)
{
    auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());
    if (!node)
    {
        qWarning(Struct_Log).noquote() << title() << "Bad node pointer process_scheme_items_hash";
        return;
    }

    bool is_cant_edit = need_to_use_parent_table(struct_type);
    const Scheme_Info scheme_info = get_scheme_info(struct_type);

    switch (struct_type)
    {
    case ST_AUTH_GROUP:
    case ST_AUTH_GROUP_PERMISSION:
    case ST_USER:
    case ST_USER_GROUP:
        is_cant_edit = true;
        break;
    default:
        break;
    }

    db_thread()->add([node, client_hash_map_arg, struct_type, is_cant_edit, scheme_info](Base* db) mutable
    {
        std::shared_ptr<Protocol> scheme = std::dynamic_pointer_cast<Protocol>(node->protocol());
        if (!scheme)
            return;

        std::lock_guard lock(node->record_mutex_);

        auto start_point = std::chrono::system_clock::now();

        Structure_Synchronizer* self = scheme->structure_sync();
        QMap<uint32_t,uint16_t> hash_map = self->get_structure_hash_map_by_type(struct_type, *db, scheme_info);
        QMap<uint32_t,uint16_t>::iterator hash_it;

        QVector<uint32_t> insert_id_vect, update_id_vect, del_id_vect;

        for (auto cl_it = client_hash_map_arg.begin(); cl_it != client_hash_map_arg.end(); )
        {
            hash_it = hash_map.find(cl_it.key());
            if (hash_it != hash_map.end())
            {
                if (hash_it.value() != cl_it.value())
                {
                    update_id_vect.push_back(cl_it.key());
                }
                hash_map.erase(hash_it);
            }
            else
            {
                insert_id_vect.push_back(cl_it.key());
            }

            cl_it = client_hash_map_arg.erase(cl_it);
        }

        for (auto hash_it = hash_map.cbegin(); hash_it != hash_map.cend(); ++hash_it)
        {
            del_id_vect.push_back(hash_it.key());
        }

        if (!insert_id_vect.isEmpty() || !update_id_vect.isEmpty() || !del_id_vect.isEmpty())
            qCDebug(Struct_Log).noquote() << scheme->title() << '[' << type_name(struct_type) << "] Check items hash. is_cant_edit:" << is_cant_edit
                               << "diffrents: update_id_vect" << update_id_vect.size() << "insert_id_vect" << insert_id_vect.size() << "del_id_vect" << del_id_vect.size();

        if (is_cant_edit)
        {
            // insert_id_vect   - delete from client
            // update_id_vect   - send data from db_name to client for update
            // del_id_vect      - send data from db_name to client for insert
            if (!insert_id_vect.isEmpty() || !update_id_vect.isEmpty() || !del_id_vect.isEmpty())
            {
                Helpz::Net::Protocol_Sender sender = scheme->send(Cmd::MODIFY_SCHEME);
                sender.timeout(nullptr, std::chrono::seconds(13));
                sender << uint32_t(0) << struct_type;
                self->add_structure_items_data(struct_type, update_id_vect, sender, *db, scheme_info);
                self->add_structure_items_data(struct_type, del_id_vect, sender, *db, scheme_info);
                sender << insert_id_vect;
            }
            else
            {
                self->set_synchronized(struct_type);
            }
        }
        else
        {
            // insert_id_vect   - get from client
            // update_id_vect   - get from client
            // del_id_vect      - delete from db_name
            if (!del_id_vect.isEmpty())
            {
                if (!self->remove_scheme_rows(*db, struct_type, del_id_vect))
                {
                    // TODO: print message
                }
            }

            insert_id_vect += update_id_vect;
            if (!insert_id_vect.isEmpty())
            {
                self->send_scheme_request(struct_type | ST_ITEM_FLAG) << insert_id_vect;
            }
            else
            {
                self->set_synchronized(struct_type);
            }
        }

        std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_point);
        if (delta > std::chrono::milliseconds(100))
        {
            std::cout << "-----------> db freeze " << delta.count() << "ms Structure_Synchronizer::process_scheme_items_hash struct_type "
                      << int(struct_type) << " " << scheme_info.ids_to_sql().toStdString() << std::endl;
        }
    });
}

void Structure_Synchronizer::process_scheme_hash(const QByteArray& client_hash, uint8_t struct_type)
{
    auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());
    if (!node)
    {
        qWarning(Struct_Log).noquote() << title() << "Bad node pointer process_scheme_hash";
        return;
    }

    const Scheme_Info scheme_info = get_scheme_info(struct_type);

    db_thread()->add([node, client_hash, struct_type, scheme_info](Base* db)
    {
        std::shared_ptr<Protocol> scheme = std::dynamic_pointer_cast<Protocol>(node->protocol());
        if (!scheme)
            return;

        std::lock_guard lock(node->record_mutex_);

        auto start_point = std::chrono::system_clock::now();

        Structure_Synchronizer* self = scheme->structure_sync();
        if (struct_type)
        {
            if (client_hash != self->get_structure_hash(struct_type, *db, scheme_info))
            {
                qCDebug(Struct_Log).noquote() << scheme->title() << '[' << type_name(struct_type) << "] Hash is diffrent." << scheme_info.ids_to_sql();
                self->send_scheme_request(struct_type | ST_FLAGS);
            }
            else
            {
                self->set_synchronized(struct_type);
            }
        }
        else
        {
            std::vector<uint8_t> struct_type_array = self->get_main_table_types();

            if (client_hash != self->get_structure_hash_for_all(*db, scheme_info))
            {
                qCDebug(Struct_Log).noquote() << scheme->title() << '[' << type_name(struct_type) << "] Common hash is diffrent." << scheme_info.ids_to_sql();
                for (const uint8_t struct_type_item: struct_type_array)
                {
                    self->send_scheme_request(struct_type_item | ST_HASH_FLAG);
                }
            }
            else
            {
                for (const uint8_t struct_type_item: struct_type_array)
                {
                    self->set_synchronized(struct_type_item);
                }
            }
        }

        std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_point);
        if (delta > std::chrono::milliseconds(100))
        {
            std::cout << "-----------> db freeze " << delta.count() << "ms Structure_Synchronizer::process_scheme_hash"
                                                                       " struct_type "
                      << int(struct_type) << " " << scheme_info.ids_to_sql().toStdString() << std::endl;
        }
    });
}

void Structure_Synchronizer::process_scheme_data(uint8_t struct_type, QIODevice* data_dev, bool delete_if_not_exist)
{
    const Scheme_Info scheme_info = get_scheme_info(struct_type);

    using T = Structure_Synchronizer;
    switch (struct_type)
    {
    case ST_DEVICE:                apply_parse(*data_dev, &T::sync_table<Device>                       , scheme_info, delete_if_not_exist); break;
    case ST_PLUGIN_TYPE:           apply_parse(*data_dev, &T::sync_table<Plugin_Type>                  , scheme_info, delete_if_not_exist); break;
    case ST_DEVICE_ITEM:           apply_parse(*data_dev, &T::sync_table<DB::Device_Item>              , scheme_info, delete_if_not_exist); break;
    case ST_DEVICE_ITEM_TYPE:      apply_parse(*data_dev, &T::sync_table<Device_Item_Type>             , scheme_info, delete_if_not_exist); break;
    case ST_SAVE_TIMER:            apply_parse(*data_dev, &T::sync_table<Save_Timer>                   , scheme_info, delete_if_not_exist); break;
    case ST_SECTION:               apply_parse(*data_dev, &T::sync_table<Section>                      , scheme_info, delete_if_not_exist); break;
    case ST_DEVICE_ITEM_GROUP:     apply_parse(*data_dev, &T::sync_table<DB::Device_Item_Group>        , scheme_info, delete_if_not_exist); break;
    case ST_DIG_TYPE:              apply_parse(*data_dev, &T::sync_table<DIG_Type>                     , scheme_info, delete_if_not_exist); break;
    case ST_DIG_MODE_TYPE:         apply_parse(*data_dev, &T::sync_table<DIG_Mode_Type>                , scheme_info, delete_if_not_exist); break;
    case ST_DIG_PARAM_TYPE:        apply_parse(*data_dev, &T::sync_table<DIG_Param_Type>               , scheme_info, delete_if_not_exist); break;
    case ST_DIG_STATUS_TYPE:       apply_parse(*data_dev, &T::sync_table<DIG_Status_Type>              , scheme_info, delete_if_not_exist); break;
    case ST_DIG_STATUS_CATEGORY:   apply_parse(*data_dev, &T::sync_table<DIG_Status_Category>          , scheme_info, delete_if_not_exist); break;
    case ST_DIG_PARAM:             apply_parse(*data_dev, &T::sync_table<DIG_Param>                    , scheme_info, delete_if_not_exist); break;
    case ST_SIGN_TYPE:             apply_parse(*data_dev, &T::sync_table<Sign_Type>                    , scheme_info, delete_if_not_exist); break;
    case ST_CODE_ITEM:             apply_parse(*data_dev, &T::sync_table<Code_Item>                    , scheme_info, delete_if_not_exist); break;
    case ST_TRANSLATION:           apply_parse(*data_dev, &T::sync_table<Translation>                  , scheme_info, delete_if_not_exist); break;
    case ST_NODE:                  apply_parse(*data_dev, &T::sync_table<DB::Node>                     , scheme_info, delete_if_not_exist); break;
    case ST_DISABLED_PARAM:        apply_parse(*data_dev, &T::sync_table<DB::Disabled_Param>           , scheme_info, delete_if_not_exist); break;
    case ST_DISABLED_STATUS:       apply_parse(*data_dev, &T::sync_table<DB::Disabled_Status>          , scheme_info, delete_if_not_exist); break;
    case ST_CHART:                 apply_parse(*data_dev, &T::sync_table<DB::Chart>                    , scheme_info, delete_if_not_exist); break;
    case ST_CHART_ITEM:            apply_parse(*data_dev, &T::sync_table<DB::Chart_Item>               , scheme_info, delete_if_not_exist); break;
    case ST_VALUE_VIEW:            apply_parse(*data_dev, &T::sync_table<DB::Value_View>               , scheme_info, delete_if_not_exist); break;
//    case ST_AUTH_GROUP:            apply_parse(*data_dev, &T::sync_table<Auth_Group>                   , scheme_info, delete_if_not_exist); break;
//    case ST_AUTH_GROUP_PERMISSION: apply_parse(*data_dev, &T::sync_table<Auth_Group_Permission>        , scheme_info, delete_if_not_exist); break;
//    case ST_USER:                  apply_parse(*data_dev, &T::sync_table<User>                         , scheme_info, delete_if_not_exist); break;
//    case ST_USER_GROUP:            apply_parse(*data_dev, &T::sync_table<User_Groups>                  , scheme_info, delete_if_not_exist); break;

    case ST_DEVICE_ITEM_VALUE:     apply_parse(*data_dev, &T::sync_table<Device_Item_Value>            , scheme_info, delete_if_not_exist); break;
    case ST_DIG_MODE:              apply_parse(*data_dev, &T::sync_table<DIG_Mode>                     , scheme_info, delete_if_not_exist); break;
    case ST_DIG_PARAM_VALUE:       apply_parse(*data_dev, &T::sync_table<DIG_Param_Value>              , scheme_info, delete_if_not_exist); break;

    default:
        qWarning(Struct_Log) << "process_scheme_data unknown type: " << int(struct_type) << scheme_info.ids_to_sql();
        return;
    }
    set_synchronized(struct_type);
}

bool Structure_Synchronizer::remove_scheme_rows(Base& db, uint8_t struct_type, const QVector<uint32_t>& delete_vect)
{
    const Scheme_Info scheme_info = get_scheme_info(struct_type);

    switch (struct_type)
    {
    case ST_DEVICE:                return DB::db_delete_rows<Device>                     (db, delete_vect, scheme_info); break;
    case ST_PLUGIN_TYPE:           return DB::db_delete_rows<Plugin_Type>                (db, delete_vect, scheme_info); break;
    case ST_DEVICE_ITEM:           return DB::db_delete_rows<DB::Device_Item>            (db, delete_vect, scheme_info); break;
    case ST_DEVICE_ITEM_TYPE:      return DB::db_delete_rows<Device_Item_Type>           (db, delete_vect, scheme_info); break;
    case ST_SAVE_TIMER:            return DB::db_delete_rows<Save_Timer>                 (db, delete_vect, scheme_info); break;
    case ST_SECTION:               return DB::db_delete_rows<Section>                    (db, delete_vect, scheme_info); break;
    case ST_DEVICE_ITEM_GROUP:     return DB::db_delete_rows<DB::Device_Item_Group>      (db, delete_vect, scheme_info); break;
    case ST_DIG_TYPE:              return DB::db_delete_rows<DIG_Type>                   (db, delete_vect, scheme_info); break;
    case ST_DIG_MODE_TYPE:         return DB::db_delete_rows<DIG_Mode_Type>              (db, delete_vect, scheme_info); break;
    case ST_DIG_PARAM_TYPE:        return DB::db_delete_rows<DIG_Param_Type>             (db, delete_vect, scheme_info); break;
    case ST_DIG_STATUS_TYPE:       return DB::db_delete_rows<DIG_Status_Type>            (db, delete_vect, scheme_info); break;
    case ST_DIG_STATUS_CATEGORY:   return DB::db_delete_rows<DIG_Status_Category>        (db, delete_vect, scheme_info); break;
    case ST_DIG_PARAM:             return DB::db_delete_rows<DIG_Param>                  (db, delete_vect, scheme_info); break;
    case ST_SIGN_TYPE:             return DB::db_delete_rows<Sign_Type>                  (db, delete_vect, scheme_info); break;
    case ST_CODE_ITEM:             return DB::db_delete_rows<Code_Item>                  (db, delete_vect, scheme_info); break;
    case ST_TRANSLATION:           return DB::db_delete_rows<Translation>                (db, delete_vect, scheme_info); break;
    case ST_NODE:                  return DB::db_delete_rows<DB::Node>                   (db, delete_vect, scheme_info); break;
    case ST_DISABLED_PARAM:        return DB::db_delete_rows<DB::Disabled_Param>         (db, delete_vect, scheme_info); break;
    case ST_DISABLED_STATUS:       return DB::db_delete_rows<DB::Disabled_Status>        (db, delete_vect, scheme_info); break;
    case ST_CHART:                 return DB::db_delete_rows<DB::Chart>                  (db, delete_vect, scheme_info); break;
    case ST_CHART_ITEM:            return DB::db_delete_rows<DB::Chart_Item>             (db, delete_vect, scheme_info); break;
    case ST_VALUE_VIEW:            return DB::db_delete_rows<DB::Value_View>             (db, delete_vect, scheme_info); break;
//    case ST_AUTH_GROUP:            return DB::db_delete_rows<Auth_Group>                 (db, delete_vect, scheme_info); break;
//    case ST_AUTH_GROUP_PERMISSION: return DB::db_delete_rows<Auth_Group_Permission>      (db, delete_vect, scheme_info); break;
//    case ST_USER:                  return DB::db_delete_rows<User>                       (db, delete_vect, scheme_info); break;
//    case ST_USER_GROUP:            return DB::db_delete_rows<User_Groups>                (db, delete_vect, scheme_info); break;

    case ST_DEVICE_ITEM_VALUE:     return DB::db_delete_rows<Device_Item_Value>          (db, delete_vect, scheme_info); break;
    case ST_DIG_MODE:              return DB::db_delete_rows<DIG_Mode>                   (db, delete_vect, scheme_info); break;
    case ST_DIG_PARAM_VALUE:       return DB::db_delete_rows<DIG_Param_Value>            (db, delete_vect, scheme_info); break;

    default:
        qWarning(Struct_Log) << "remove_scheme_rows unknown type: " << int(struct_type);
        break;
    }
    return false;
}

/*
void Structure_Synchronizer::check_codes_checksum(const QVector<Code_Summ>& sum_list)
{
    QVector<Code_Item> code_list = db()->getCode(name()), upd_code_list;
    QVector<Code_Item>::iterator it;

    QVector<uint32_t> id_required;

    for (const Code_Summ& code_summ: sum_list)
    {
        it = std::find_if(code_list.begin(), code_list.end(), [&code_summ](const Code_Item& code)
        {
            return code.id() == code_summ.id();
        });

        if (it == code_list.end())
        {
#pragma GCC warning "Check if global_id exist and used for scheme_group"
//            if (!code_summ.global_id) {
//            }
            id_required.push_back(code_summ.id());
        }
        else
        {
            if (code_summ.checksum != Code_Summ::get_checksum(*it))
            {
                upd_code_list.push_back(std::move(*it));
            }
            code_list.erase(it);
        }
    }

    //    std::cout << title() << " check_codes_checksum " << " get: " << id_required.size() << " upd " << upd_code_list.size() << " ins " << code_list.size() << std::endl;

    if (id_required.size())
    {
        send_scheme_request(ST_SCRIPTS) << id_required; // get code from client
    }

    if (upd_code_list.size() || code_list.size())
    {
        protocol_->send(Cmd::MODIFY_SCHEME) << uint32_t(0) << uint8_t(ST_SCRIPTS) << upd_code_list << code_list << QVector<uint32_t>{}; // send code to client
    }
}

void Structure_Synchronizer::set_codes(QVector<Code_Item>&& codes)
{
    QVector<Code_Item> code_list = db()->getCode(name());
    codes.erase(std::remove_if(codes.begin(), codes.end(),
                               [&code_list](Code_Item& code)
    {
        return !code.id() || std::find_if(code_list.cbegin(), code_list.cend(),
        [&code](const Code_Item& code_1) { return code.id() == code_1.id(); }) != code_list.cend();
    }), codes.end());

    code_list += codes;

    for (Code_Item& code: code_list)
    {
        if (code.global_id)
        {
#pragma GCC warning "TODO: Check if global_id exist and used for scheme_group"
            code.set_name(QString());
            code.text.clear();
        }
    }

    sync_table(std::move(code_list));
}*/

QString Structure_Synchronizer::get_auth_user_suffix(const QString& user_id_str) const
{
    QString sql = "LEFT JOIN das_scheme_group_user sgu ON sgu.user_id = ";
    sql += user_id_str;
    sql += " WHERE sgu.group_id IN (";
    for (uint32_t scheme_group_id: protocol_->scheme_groups())
        sql += QString::number(scheme_group_id) + ',';
    sql[sql.size() - 1] = ')';
    return sql;
}

void Structure_Synchronizer::fill_suffix(uint8_t struct_type, QString& where_str)
{
    QString suffix, group_by = " GROUP BY ";
    switch (struct_type)
    {
    case ST_USER:
        suffix = get_auth_user_suffix(User::table_short_name() + ".id");
        group_by += User::table_short_name();
        break;
    case ST_USER_GROUP:
        suffix = get_auth_user_suffix(User_Groups::table_short_name() + ".user_id");
        group_by += User_Groups::table_short_name();
        break;
    default: break;
    }

    if (!suffix.isEmpty())
    {
        if (where_str.isEmpty())
        {
            where_str = std::move(suffix);
        }
        else
        {
            suffix += " AND";
            where_str.replace("WHERE", suffix);
        }

        where_str += group_by;
        where_str += ".id"; // for GROUP BY ?.id
    }
}

bool Structure_Synchronizer::is_can_modify(uint8_t struct_type) const
{
    switch (struct_type)
    {
    case ST_AUTH_GROUP:
    case ST_AUTH_GROUP_PERMISSION:
    case ST_USER:
    case ST_USER_GROUP:
        return false;
    default:
        break;
    }

    return !need_to_use_parent_table(struct_type);
}

//template<typename T> class Sync_Process_Helper;

//template<typename T>
//class Sync_Process_Helper
//{
//public:
//    Sync_Process_Helper(Structure_Synchronizer* /*sync*/) {}
//    void remove(QSqlQuery& /*query*/) {}
//    void update(const T& /*item*/) {}
//    void insert(const QVector<T>& /*items*/) {}
//};

template<class T>
bool Structure_Synchronizer::sync_table(QVector<T>&& items, const Scheme_Info &scheme, bool delete_if_not_exist)
{
#pragma GCC warning "Check if Code_Item::global_id exist and used for scheme_group"
//    Sync_Process_Helper<T> sync_proc_helpher(this);

    using Helper = DB::Scheme_Table_Helper<T>;
    using PK_Type = typename Helper::PK_Type;

    PK_Type id_value;
    std::vector<Update_Info> update_list;
    QVector<PK_Type> delete_vect;

    Table table = db_table<T>();

    std::shared_ptr<DB::global> db_ptr = db();
    {
        int compare_column_count = table.field_names().size();

        QString suffix;
        if (DB::has_scheme_id<T>())
        {
            suffix = "WHERE " + scheme.ids_to_sql();
            --compare_column_count;
        }

        QSqlQuery q = db_ptr->select(table, suffix);
        if (!q.isActive())
        {
            qWarning() << "sync_table: Failed select from" << table.name() << scheme.ids_to_sql();
            return false;
        }

        QVariant sql_value, client_value;
        while (q.next())
        {
            id_value = Helper::get_query_pk(q);

            for (auto it = items.begin(); it <= items.end(); it++)
            {
                if (it == items.end())
                {
                    if (delete_if_not_exist)
                    {
//                        sync_proc_helpher.remove(q);
                        delete_vect.push_back(id_value);
                    }
                }
                else if (id_value == Helper::get_pk(*it))
                {
                    Update_Info ui;

                    for (int pos = 1; pos < compare_column_count; ++pos)
                    {
                        sql_value = q.isNull(pos) ? QVariant() : q.value(pos);
                        client_value = T::value_getter(*it, pos);
                        if (sql_value != client_value)
                        {
                            ui.changed_field_names_.push_back(table.field_names().at(pos));
                            ui.changed_fields_.push_back(client_value);
                        }
                        else if (sql_value.type() != client_value.type() && sql_value.toString() != client_value.toString())
                        {
                            qDebug() << typeid(T).name() << table.name() << table.field_names().at(pos)
                                     << "is same value but diffrent types" << sql_value << client_value;
                        }
                    }

                    if (!ui.changed_fields_.empty())
                    {
//                        sync_proc_helpher.update(*it);

                        ui.changed_fields_.push_back(id_value);
                        update_list.push_back(std::move(ui));
                    }

                    items.erase(it--);
                    break;
                }
            }
        }
    }

    Uncheck_Foreign uncheck_foreign(db_ptr.get());

    if (delete_if_not_exist && !delete_vect.isEmpty())
    {
        if (!DB::db_delete_rows<T>(*db_ptr.get(), delete_vect, scheme))
        {
            qWarning() << "sync_table: Failed delete row in" << table.name() << scheme.ids_to_sql();
            return false;
        }
    }

    if (!update_list.empty())
    {
        const QString where = table.field_names().at(Helper::pk_num) + "=?" +
                (DB::has_scheme_id<T>() ? " AND scheme_id=" + QString::number(scheme.id()) : QString());

        Table upd_table{table.name(), {}, {}};

        for (const Update_Info& ui: update_list)
        {
            upd_table.set_field_names(ui.changed_field_names_);
            if (!db_ptr->update(upd_table, ui.changed_fields_, where).isActive())
            {
                qWarning() << "sync_table: Failed update row in" << table.name() << scheme.ids_to_sql();
                return false;
            }
        }
    }

    if (!items.empty())
    {
//        sync_proc_helpher.insert(items);

        if (Helper::pk_num != 0)
            table.field_names().removeFirst();

        QVariantList values;
        for (T& item: items)
        {
            if constexpr (DB::has_scheme_id<T>())
                item.set_scheme_id(scheme.id());

            values = T::to_variantlist(item);
            if (Helper::pk_num != 0)
                values.removeFirst();

            if (!db_ptr->insert(table, values))
            {
                qWarning() << "sync_table: Failed insert row to" << table.name() << scheme.ids_to_sql();
                return false;
            }
        }
    }

    return true;
}

} // namespace Server
} // namespace Ver
} // namespace Das
