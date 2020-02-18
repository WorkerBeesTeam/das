#include <iostream>

#include <Helpz/db_builder.h>
#include <Helpz/dtls_server_thread.h>

#include <Das/commands.h>
#include <Das/db/translation.h>

#include <plus/das/database_delete_info.h>

#include "server.h"
#include "server_protocol_2_1.h"
#include "dbus_object.h"
#include "structure_synchronizer_2_1.h"

namespace Das {
namespace Ver_2_1 {
namespace Server {

Structure_Synchronizer::Structure_Synchronizer(Protocol_Base *protocol) :
    Base_Synchronizer(protocol),
    Structure_Synchronizer_Base(protocol->db_thread()),
    struct_sync_timeout_(false)
{
}

void Structure_Synchronizer::check(bool custom_only)
{
    if (!custom_only)
        send_scheme_request(STRUCT_TYPE_HASH_FLAG);

    std::vector<uint8_t> struct_type_array = get_main_table_types();
    std::vector<uint8_t>::iterator it;

    for (uint8_t i = 1; i < STRUCT_TYPE_COUNT; ++i)
    {
        if (struct_type_array.size())
        {
            it = std::find(struct_type_array.begin(), struct_type_array.end(), i);
            if (it != struct_type_array.end())
            {
                struct_type_array.erase(it);
                continue;
            }
        }
        send_scheme_request(i | STRUCT_TYPE_HASH_FLAG);
    }
}

void Structure_Synchronizer::process_modify(uint32_t user_id, uint8_t struct_type, QIODevice *data_dev)
{
    if (protocol_->parent_id() && is_main_table(struct_type))
    {
        return;
    }

    if (struct_type == STRUCT_TYPE_GROUP_STATUS)
    {
        QVector<Ver_2_1_DIG_Status> upd_vect, insrt_vect, del_vect;
        QVector<uint32_t> del_id_vect;

        qint64 pos = data_dev->pos();
        Helpz::parse_out(Helpz::Network::Protocol::DATASTREAM_VERSION, *data_dev, upd_vect, insrt_vect, del_id_vect);
        data_dev->seek(pos);

        upd_vect += insrt_vect;

        if (!upd_vect.isEmpty() || !del_id_vect.isEmpty())
        {
            if (!del_id_vect.isEmpty())
            {
                auto db = protocol_->db();
                del_vect = Helpz::DB::db_build_list<Ver_2_1_DIG_Status>(*db, del_id_vect, Database::scheme::db_name(name()));
            }
            process_statuses(std::move(upd_vect), std::move(del_vect));
        }
    }

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
        qWarning(Struct_Log) << "Bad node pointer process_modify";
        return;
    }
    process_modify_message(user_id, struct_type, data_dev, Database::scheme::db_name(name()), [node]() -> std::shared_ptr<Bad_Fix>
    {
        std::shared_ptr<Server_Bad_Fix> server_bad_fix(new Server_Bad_Fix);
        server_bad_fix->node_ = node;

        return std::static_pointer_cast<Bad_Fix>(server_bad_fix);
    });
}

void Structure_Synchronizer::process_statuses(QVector<Ver_2_1_DIG_Status>&& ups_vect, QVector<Ver_2_1_DIG_Status>&& del_vect)
{
    QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "process_statuses", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *protocol_),
                              Q_ARG(QVector<DIG_Status>, reinterpret_cast<const QVector<DIG_Status>&>(ups_vect)),
                              Q_ARG(QVector<DIG_Status>, reinterpret_cast<const QVector<DIG_Status>&>(del_vect)),
                              Q_ARG(QString, get_db_name(STRUCT_TYPE_GROUP_DIG_STATUS_TYPE)));
}

bool Structure_Synchronizer::need_to_use_parent_table(uint8_t struct_type) const
{
    switch (struct_type)
    {
    case STRUCT_TYPE_AUTH_GROUP:
    case STRUCT_TYPE_AUTH_GROUP_PERMISSION:
    case STRUCT_TYPE_AUTH_USER:
    case STRUCT_TYPE_AUTH_USER_GROUP:
        return true;
    default: break;
    }

    return protocol_->parent_id() && is_main_table(struct_type);
}

QString Structure_Synchronizer::get_db_name(uint8_t struct_type) const
{
    switch (struct_type)
    {
    case STRUCT_TYPE_AUTH_GROUP:
    case STRUCT_TYPE_AUTH_GROUP_PERMISSION:
    case STRUCT_TYPE_AUTH_USER:
    case STRUCT_TYPE_AUTH_USER_GROUP:
        return common_db_name();
    default: break;
    }

    if (protocol_->parent_id() && is_main_table(struct_type))
    {
        return Database::scheme::db_name(protocol_->parent_name());
    }
    else
    {
        return Database::scheme::db_name(name());
    }
}

void Structure_Synchronizer::send_modify_response(uint8_t /*struct_type*/, const QByteArray& buffer, uint32_t /*user_id*/)
{
    QMetaObject::invokeMethod(protocol()->work_object()->dbus_, "structure_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, *protocol_), Q_ARG(QByteArray, buffer));
}

Helpz::Network::Protocol_Sender Structure_Synchronizer::send_scheme_request(uint8_t struct_type)
{
    if (!struct_sync_timeout_)
    {
        struct_wait_set_.insert(struct_type);
    }

    Helpz::Network::Protocol_Sender sender = std::move(protocol_->send(Cmd::GET_SCHEME)
            .answer([this](QIODevice& data_dev)
    {
        apply_parse(data_dev, &Structure_Synchronizer::process_scheme, &data_dev);
    })
            .timeout([this, struct_type]()
    {
        struct_sync_timeout_ = true;
        struct_wait_set_.clear();

        uint8_t flags = struct_type & STRUCT_TYPE_FLAGS;
        uint8_t st_type = struct_type & ~STRUCT_TYPE_FLAGS;
        qCWarning(Struct_Log).noquote() << title() << "[" << static_cast<StructureType>(st_type) << "] Timeout. flags:" << flags;
        protocol()->set_connection_state(CS_CONNECTED_SYNC_TIMEOUT | (modified() ? CS_CONNECTED_MODIFIED : 0));
    }, std::chrono::minutes(5), std::chrono::seconds(30)));

    sender << struct_type;

    return sender;
}

void Structure_Synchronizer::process_scheme(uint8_t struct_type, QIODevice* data_dev)
{
//    qCDebug(Struct_Log) << title().c_str() << "process_scheme" << static_cast<StructureType>(struct_type & ~STRUCT_TYPE_HASH_FLAG) << "hash"
//             << bool(struct_type & STRUCT_TYPE_HASH_FLAG) << "size" << data_dev->size();

    uint8_t flags = struct_type & STRUCT_TYPE_FLAGS;
    struct_type &= ~STRUCT_TYPE_FLAGS;

    struct_wait_set_.erase(struct_type);
    struct_wait_set_.erase(struct_type | flags);

    if (flags & STRUCT_TYPE_HASH_FLAG)
    {
        if (flags & STRUCT_TYPE_ITEM_FLAG)
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
        process_scheme_data(struct_type, data_dev, (flags & STRUCT_TYPE_ITEM_FLAG) == 0);
    }
    else
    {
        qCWarning(Struct_Log) << "need_to_use_parent_table" << struct_type << "flags" << flags;
    }

    if (!struct_sync_timeout_ && struct_wait_set_.empty())
    {
        protocol()->set_connection_state(CS_CONNECTED | (modified() ? CS_CONNECTED_MODIFIED : 0));
    }
}

void Structure_Synchronizer::process_scheme_items_hash(QVector<QPair<uint32_t,uint16_t>>&& client_hash_vect, uint8_t struct_type)
{
    bool is_child = need_to_use_parent_table(struct_type);
    if (is_child && protocol()->parent_name().isEmpty())
    {
        qCCritical(Struct_Log).noquote() << protocol()->title() << "scheme has parent_id" << protocol()->parent_id() << "but parent_name is empty for struct_type" << struct_type;
        return;
    }

    QString db_name = Database::scheme::db_name(is_child ? protocol()->parent_name() : protocol()->name());

    auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());
    if (!node)
    {
        qWarning(Struct_Log) << "Bad node pointer process_scheme_items_hash";
        return;
    }
    else
    {
        db_thread()->add([node, client_hash_vect, struct_type, is_child, db_name](Helpz::DB::Base* db) mutable
        {
            auto start_point = std::chrono::system_clock::now();

            std::shared_ptr<Protocol> scheme = std::dynamic_pointer_cast<Protocol>(node->protocol());
            if (scheme)
            {
                Structure_Synchronizer* self = scheme->structure_sync();
                QVector<QPair<uint32_t,uint16_t>> hash_vect = self->get_structure_hash_vect_by_type(struct_type, *db, db_name);
                QVector<QPair<uint32_t,uint16_t>>::iterator hash_it;

                QVector<uint32_t> insert_id_vect, update_id_vect, del_id_vect;

                for (auto cl_it = client_hash_vect.begin(); cl_it != client_hash_vect.end(); )
                {
                    hash_it = hash_vect.end();
                    for (auto it = hash_vect.begin(); it != hash_vect.end(); ++it)
                    {
                        if (it->first == cl_it->first)
                        {
                            hash_it = it;
                            break;
                        }
                    }

                    if (hash_it != hash_vect.end())
                    {
                        if (hash_it->second != cl_it->second)
                        {
                            update_id_vect.push_back(cl_it->first);
                        }
                        hash_vect.erase(hash_it);
                    }
                    else
                    {
                        insert_id_vect.push_back(cl_it->first);
                    }

                    cl_it = client_hash_vect.erase(cl_it);
                }

                for (const QPair<uint32_t,uint16_t>& hash_pair: hash_vect)
                {
                    del_id_vect.push_back(hash_pair.first);
                }

                if (!insert_id_vect.isEmpty() || !update_id_vect.isEmpty() || !del_id_vect.isEmpty())
                    qCDebug(Struct_Log).noquote() << scheme->title() << "[" << static_cast<StructureType>(struct_type) << "] Check items hash. is_child:" << is_child
                                       << "diffrents: update_id_vect" << update_id_vect.size() << "insert_id_vect" << insert_id_vect.size() << "del_id_vect" << del_id_vect.size();

                if (is_child)
                {
                    // insert_id_vect   - delete from client
                    // update_id_vect   - send data from db_name to client for update
                    // del_id_vect      - send data from db_name to client for insert
                    if (!insert_id_vect.isEmpty() || !update_id_vect.isEmpty() || !del_id_vect.isEmpty())
                    {
                        Helpz::Network::Protocol_Sender sender = scheme->send(Cmd::MODIFY_SCHEME);
                        sender << uint32_t(0) << struct_type;
                        self->add_structure_items_data(struct_type, update_id_vect, sender, *db, db_name);
                        self->add_structure_items_data(struct_type, del_id_vect, sender, *db, db_name);
                        sender << insert_id_vect;
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
                        self->send_scheme_request(struct_type | STRUCT_TYPE_ITEM_FLAG) << insert_id_vect;
                    }
                }
            }

            std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_point);
            if (delta > std::chrono::milliseconds(100))
            {
                std::cout << "-----------> db freeze " << delta.count() << "ms Structure_Synchronizer::process_scheme_items_hash struct_type "
                          << (struct_type) << " name " << db_name.toStdString() << std::endl;
            }
        });
    }
}

void Structure_Synchronizer::process_scheme_hash(const QByteArray& client_hash, uint8_t struct_type)
{
    QString db_name = get_db_name(struct_type);

    auto node = std::dynamic_pointer_cast<Helpz::DTLS::Server_Node>(protocol()->writer());
    if (!node)
    {
        qWarning(Struct_Log) << "Bad node pointer process_scheme_hash";
        return;
    }
    else
    {
        db_thread()->add([node, client_hash, struct_type, db_name](Helpz::DB::Base* db)
        {
            auto start_point = std::chrono::system_clock::now();

            std::shared_ptr<Protocol> scheme = std::dynamic_pointer_cast<Protocol>(node->protocol());
            if (scheme)
            {
                Structure_Synchronizer* self = scheme->structure_sync();
                if (struct_type)
                {
                    if (client_hash != self->get_structure_hash(struct_type, *db, db_name))
                    {
                        qCDebug(Struct_Log).noquote() << scheme->title() << '[' << static_cast<StructureType>(struct_type) << "] Hash is diffrent. DB:" << db_name;
                        self->send_scheme_request(struct_type | STRUCT_TYPE_FLAGS);
                    }
                }
                else
                {
                    if (client_hash != self->get_structure_hash_for_all(*db, db_name))
                    {
                        qCDebug(Struct_Log).noquote() << scheme->title() << '[' << static_cast<StructureType>(struct_type) << "] Common hash is diffrent. DB:" << db_name;
                        std::vector<uint8_t> struct_type_array = self->get_main_table_types();
                        for (const uint8_t struct_type_item: struct_type_array)
                        {
                            self->send_scheme_request(struct_type_item | STRUCT_TYPE_FLAGS);
                        }
                    }
                }
            }

            std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_point);
            if (delta > std::chrono::milliseconds(100))
            {
                std::cout << "-----------> db freeze " << delta.count() << "ms Structure_Synchronizer::process_scheme_hash"
                                                                           " struct_type "
                          << (struct_type) << " name " << db_name.toStdString() << std::endl;
            }
        });
    }
}

void Structure_Synchronizer::process_scheme_data(uint8_t struct_type, QIODevice* data_dev, bool delete_if_not_exist)
{
    switch (struct_type)
    {
    case STRUCT_TYPE_DEVICES:               apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Device>                  , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_CHECKER_TYPES:         apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Plugin_Type>             , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_DEVICE_ITEMS:          apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Database::Device_Item>   , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_DEVICE_Device_ITEM_TYPES:     apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Device_Item_Type>               , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_SAVE_TIMERS:           apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Save_Timer>              , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_SECTIONS:              apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Section>                 , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_GROUPS:                apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Database::Device_Item_Group>    , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_GROUP_TYPES:           apply_parse(*data_dev, &Structure_Synchronizer::sync_table<DIG_Type>         , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_GROUP_DIG_MODES:      apply_parse(*data_dev, &Structure_Synchronizer::sync_table<DIG_Mode>               , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_GROUP_DIG_PARAM_TYPES:     apply_parse(*data_dev, &Structure_Synchronizer::sync_table<DIG_Param_Type>              , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_TYPE:     apply_parse(*data_dev, &Structure_Synchronizer::sync_table<DIG_Status_Type>             , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_CATEGORY:     apply_parse(*data_dev, &Structure_Synchronizer::sync_table<DIG_Status_Category>             , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_DIG_PARAM:           apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Dig_Param>             , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_SIGNS:                 apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Sign_Item>               , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_VIEW:                  apply_parse(*data_dev, &Structure_Synchronizer::sync_table<View>                    , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_VIEW_ITEM:             apply_parse(*data_dev, &Structure_Synchronizer::sync_table<View_Item>               , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_SCRIPTS:               apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Code_Item>               , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_TRANSLATIONS:          apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Translation>             , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_AUTH_GROUP:            apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Auth_Group>              , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_AUTH_GROUP_PERMISSION: apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Auth_Group_Permission>   , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_AUTH_USER:             apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Auth_User>               , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_AUTH_USER_GROUP:       apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Auth_User_Group>         , get_db_name(struct_type), delete_if_not_exist); break;

    case STRUCT_TYPE_DEVICE_ITEM_VALUES:    apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Device_Item_Value>       , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_DIG_MODE_ITEM:            apply_parse(*data_dev, &Structure_Synchronizer::sync_table<DIG_Mode_Item>              , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_GROUP_STATUS:          apply_parse(*data_dev, &Structure_Synchronizer::sync_table<Ver_2_1_DIG_Status>       , get_db_name(struct_type), delete_if_not_exist); break;
    case STRUCT_TYPE_DIG_PARAM_VALUE:     apply_parse(*data_dev, &Structure_Synchronizer::sync_table<DIG_Param_Value>       , get_db_name(struct_type), delete_if_not_exist); break;

    default:
        break;
    }
}

bool Structure_Synchronizer::remove_scheme_rows(Helpz::DB::Base& db, uint8_t struct_type, const QVector<uint32_t>& delete_vect)
{
    switch (struct_type)
    {
    case STRUCT_TYPE_DEVICES:               return Database::db_delete_rows<Device>                  (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_CHECKER_TYPES:         return Database::db_delete_rows<Plugin_Type>             (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_DEVICE_ITEMS:          return Database::db_delete_rows<Database::Device_Item>   (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_DEVICE_Device_ITEM_TYPES:     return Database::db_delete_rows<Device_Item_Type>               (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_SAVE_TIMERS:           return Database::db_delete_rows<Save_Timer>              (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_SECTIONS:              return Database::db_delete_rows<Section>                 (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_GROUPS:                return Database::db_delete_rows<Database::Device_Item_Group>    (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_GROUP_TYPES:           return Database::db_delete_rows<DIG_Type>         (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_GROUP_DIG_MODES:      return Database::db_delete_rows<DIG_Mode>               (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_GROUP_DIG_PARAM_TYPES:     return Database::db_delete_rows<DIG_Param_Type>              (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_TYPE:     return Database::db_delete_rows<DIG_Status_Type>             (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_CATEGORY:     return Database::db_delete_rows<DIG_Status_Category>             (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_DIG_PARAM:           return Database::db_delete_rows<Dig_Param>             (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_SIGNS:                 return Database::db_delete_rows<Sign_Item>               (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_VIEW:                  return Database::db_delete_rows<View>                    (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_VIEW_ITEM:             return Database::db_delete_rows<View_Item>               (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_SCRIPTS:               return Database::db_delete_rows<Code_Item>               (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_TRANSLATIONS:          return Database::db_delete_rows<Translation>             (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_AUTH_GROUP:            return Database::db_delete_rows<Auth_Group>              (db, delete_vect, common_db_name()); break;
    case STRUCT_TYPE_AUTH_GROUP_PERMISSION: return Database::db_delete_rows<Auth_Group_Permission>   (db, delete_vect, common_db_name()); break;
    case STRUCT_TYPE_AUTH_USER:             return Database::db_delete_rows<Auth_User>               (db, delete_vect, common_db_name()); break;
    case STRUCT_TYPE_AUTH_USER_GROUP:       return Database::db_delete_rows<Auth_User_Group>         (db, delete_vect, common_db_name()); break;

    case STRUCT_TYPE_DEVICE_ITEM_VALUES:    return Database::db_delete_rows<Device_Item_Value>       (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_DIG_MODE_ITEM:            return Database::db_delete_rows<DIG_Mode_Item>              (db, delete_vect, Database::scheme::db_name(name())); break;
    case STRUCT_TYPE_GROUP_STATUS:          return remove_statuses(db, delete_vect); break;
    case STRUCT_TYPE_DIG_PARAM_VALUE:     return Database::db_delete_rows<DIG_Param_Value>       (db, delete_vect, Database::scheme::db_name(name())); break;

    default:
        break;
    }
    return false;
}

bool Structure_Synchronizer::remove_statuses(Helpz::DB::Base& db, const QVector<uint32_t>& delete_vect)
{
    QVector<Ver_2_1_DIG_Status> items = Helpz::DB::db_build_list<Ver_2_1_DIG_Status>(db, delete_vect, Database::scheme::db_name(name()));
    if (!items.isEmpty())
    {
        process_statuses({}, std::move(items));
    }
    // TODO: Отправить удедомление
    return Database::db_delete_rows<Ver_2_1_DIG_Status>(db, delete_vect, Database::scheme::db_name(name()));
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
        send_scheme_request(STRUCT_TYPE_SCRIPTS) << id_required; // get code from client
    }

    if (upd_code_list.size() || code_list.size())
    {
        protocol_->send(Cmd::MODIFY_SCHEME) << uint32_t(0) << uint8_t(STRUCT_TYPE_SCRIPTS) << upd_code_list << code_list << QVector<uint32_t>{}; // send code to client
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
    QString sql = " LEFT JOIN das_employee hle ON ";
    sql += user_id_str;
    sql += " = hle.user_id WHERE hle.scheme_group_id IN (";
    for (uint32_t scheme_group_id: protocol_->scheme_groups())
        sql += QString::number(scheme_group_id) + ',';
    sql[sql.size() - 1] = ')';
    return sql;
}

QString Structure_Synchronizer::get_suffix(uint8_t struct_type)
{
    switch (struct_type) {
    case STRUCT_TYPE_AUTH_USER:         return get_auth_user_suffix(Auth_User::table_short_name() + ".id");
    case STRUCT_TYPE_AUTH_USER_GROUP:   return get_auth_user_suffix(Auth_User_Group::table_short_name() + ".user_id");
    default: break;
    }
    return {};
}

template<typename T> class Sync_Process_Helper;

template<>
class Sync_Process_Helper<Ver_2_1_DIG_Status>
{
public:
    Sync_Process_Helper(Structure_Synchronizer* sync) : sync_(sync) {}
    ~Sync_Process_Helper()
    {
        if (!add_vect_.isEmpty() || !del_vect_.isEmpty())
        {
            sync_->process_statuses(std::move(add_vect_), std::move(del_vect_));
        }
    }

    void remove(QSqlQuery& query)
    {
        Ver_2_1_DIG_Status item = Helpz::DB::db_build<Ver_2_1_DIG_Status>(query);
        del_vect_.push_back(std::move(item));
    }

    void update(const Ver_2_1_DIG_Status& item)
    {
        add_vect_.push_back(item);
    }

    void insert(const QVector<Ver_2_1_DIG_Status>& items)
    {
        add_vect_ += items;
    }

    Structure_Synchronizer* sync_;

    QVector<Ver_2_1_DIG_Status> add_vect_, del_vect_;
};

template<typename T>
class Sync_Process_Helper
{
public:
    Sync_Process_Helper(Structure_Synchronizer* /*sync*/) {}
    void remove(QSqlQuery& /*query*/) {}
    void update(const T& /*item*/) {}
    void insert(const QVector<T>& /*items*/) {}
};

template<class T>
bool Structure_Synchronizer::sync_table(QVector<T>&& items, const QString& db_name, bool delete_if_not_exist)
{
#pragma GCC warning "Check if Code_Item::global_id exist and used for scheme_group"
    Sync_Process_Helper<T> sync_proc_helpher(this);

    using PK_Type = typename std::result_of<decltype(&T::id)(T)>::type;

    struct UpdateInfo
    {
        PK_Type id;
        QStringList changedFieldNames;
        QVariantList changedFields;
    };
    PK_Type id_value;
    std::vector<UpdateInfo> update_list;
    QVector<PK_Type> delete_vect;

    Helpz::DB::Table table = Helpz::DB::db_table<T>(db_name);

    std::shared_ptr<Database::global> db_ptr = db();
    {
        QSqlQuery q = db_ptr->select(table);
        if (!q.isActive())
            return false;

        QVariant value;
        while (q.next())
        {
            id_value = q.value(0).value<PK_Type>();
            for (auto it = items.begin(); it <= items.end(); it++)
            {
                if (it == items.end())
                {
                    if (delete_if_not_exist)
                    {
                        sync_proc_helpher.remove(q);
                        delete_vect.push_back(id_value);
                    }
                }
                else if (it->id() == id_value)
                {
                    UpdateInfo ui;

                    for (int pos = 1; pos < table.field_names().size(); ++pos)
                    {
                        value = T::value_getter(*it, pos);
                        if ((q.isNull(pos) ? QVariant() : q.value(pos)) != value)
                        {
                            ui.changedFieldNames.push_back(table.field_names().at(pos));
                            ui.changedFields.push_back(value);
                        }
                    }

                    if (ui.changedFields.size())
                    {
                        sync_proc_helpher.update(*it);

                        ui.id = q.value(0).toUInt();
                        update_list.push_back(ui);
                    }

                    items.erase(it--);
                    break;
                }
            }
        }
    }

    struct UncheckForeign
    {
        UncheckForeign(Helpz::DB::Base* p) : p_(p) { p->exec("SET foreign_key_checks=0;"); }
        ~UncheckForeign() { p_->exec("SET foreign_key_checks=1;"); }
        Helpz::DB::Base* p_;
    } uncheck_foreign(db_ptr.get());

    if (delete_if_not_exist && !delete_vect.isEmpty())
    {
        if (!Database::db_delete_rows<T>(*db_ptr.get(), delete_vect, Database::scheme::db_name(name())))
        {
            return false;
        }
    }

    if (!update_list.empty())
    {
        for (const UpdateInfo& ui: update_list)
            if (!db_ptr->update({table.name(), {}, ui.changedFieldNames}, ui.changedFields, table.field_names().first() + '=' + QString::number(ui.id)))
                return false;
    }

    if (!items.empty())
    {
        sync_proc_helpher.insert(items);

        QVariantList values;
        for (T& item: items)
        {
            values.clear();
            for (int pos = 0; pos < table.field_names().size(); ++pos)
                values.push_back(T::value_getter(item, pos));

            if (!db_ptr->insert(table, values))
                return false;
        }
    }

    return true;
}

} // namespace Server
} // namespace Ver_2_1
} // namespace Das
