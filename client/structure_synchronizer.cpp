#include <Helpz/db_builder.h>

#include <Das/commands.h>
#include <Das/db/translation.h>
#include <Das/db/user.h>
#include <Das/db/auth_group.h>

#include "worker.h"
#include "Network/client_protocol.h"
#include "structure_synchronizer.h"

namespace Das {
namespace Ver {
namespace Client {

Structure_Synchronizer::Structure_Synchronizer(Helpz::DB::Thread *db_thread, Protocol* protocol) :
    Structure_Synchronizer_Base(db_thread),
    protocol_(protocol)
{
    qRegisterMetaType<std::shared_ptr<Ver::Client::Protocol>>("std::shared_ptr<Ver::Client::Protocol_Base>");
}

/*static*/ QString Structure_Synchronizer::get_scheme_group_users_fill_sql()
{
    return "INSERT IGNORE INTO das_scheme_group_user (group_id, user_id) "
           "SELECT 1, u.id FROM das_user u;";
}

void Structure_Synchronizer::send_scheme_structure(uint8_t struct_type, uint8_t msg_id, QIODevice *data_dev)
{
    uint8_t flags = struct_type & ST_FLAGS;
    struct_type &= ~ST_FLAGS;

    qCDebug(Struct_Detail_Log) << "GET_SCHEME" << static_cast<Structure_Type>(struct_type)
                               << "flags:" << int(flags);

    if (flags & ST_HASH_FLAG)
    {
        if (flags & ST_ITEM_FLAG)
        {
            send_structure_items_hash(struct_type, msg_id);
        }
        else if (struct_type)
        {
            send_structure_hash(struct_type, msg_id);
        }
        else
        {
            send_structure_hash_for_all(msg_id);
        }
    }
    else
    {
        if (flags & ST_ITEM_FLAG)
        {
            Helpz::apply_parse(*data_dev, Helpz::Network::Protocol::DATASTREAM_VERSION, &Structure_Synchronizer::send_structure_items, this, struct_type, msg_id);
        }
        else
        {
            send_structure(struct_type, msg_id);
        }
    }
}

void Structure_Synchronizer::send_structure_items_hash(uint8_t struct_type, uint8_t msg_id)
{
    Worker* worker = protocol_->worker();
    db_thread()->add_query([worker, struct_type, msg_id](Helpz::DB::Base *db)
    {
        auto proto = worker->net_protocol();
        if (proto)
        {
            Helpz::Network::Protocol_Sender helper = proto->send_answer(Cmd::GET_SCHEME, msg_id);
            helper << uint8_t(struct_type | ST_FLAGS);
//            helper.timeout(nullptr, std::chrono::minutes(5), std::chrono::seconds(90)); // Пока нет возможности отправлять ответ на ответ
            proto->structure_sync().add_structure_items_hash(struct_type, helper, *db);
        }
    });
}

void Structure_Synchronizer::add_structure_items_hash(uint8_t struct_type, QDataStream& ds, Helpz::DB::Base& db)
{
    ds << get_structure_hash_map_by_type(struct_type, db, DB::Schemed_Model::default_scheme_id());
}

void Structure_Synchronizer::send_structure_items(const QVector<uint32_t>& id_vect, uint8_t struct_type, uint8_t msg_id)
{
    Worker* worker = protocol_->worker();
    db_thread()->add_query([worker, id_vect, struct_type, msg_id](Helpz::DB::Base *db)
    {
        auto proto = worker->net_protocol();
        if (proto)
        {
            Helpz::Network::Protocol_Sender helper = proto->send_answer(Cmd::GET_SCHEME, msg_id);
            helper << uint8_t(struct_type | ST_ITEM_FLAG);
//            helper.timeout(nullptr, std::chrono::minutes(5), std::chrono::seconds(90)); // Пока нет возможности отправлять ответ на ответ
            proto->structure_sync().add_structure_items_data(struct_type, id_vect, helper, *db, DB::Schemed_Model::default_scheme_id());
        }
    });
}

void Structure_Synchronizer::send_structure_hash(uint8_t struct_type, uint8_t msg_id)
{
    Worker* worker = protocol_->worker();
    db_thread()->add_query([worker, msg_id, struct_type](Helpz::DB::Base *db)
    {
        auto proto = worker->net_protocol();
        if (proto)
        {
            proto->send_answer(Cmd::GET_SCHEME, msg_id)
                    << uint8_t(struct_type | ST_HASH_FLAG)
                    << proto->structure_sync().get_structure_hash(struct_type, *db, DB::Schemed_Model::default_scheme_id());
        }
    });
}

void Structure_Synchronizer::send_structure_hash_for_all(uint8_t msg_id)
{
    Worker* worker = protocol_->worker();
    db_thread()->add_query([worker, msg_id](Helpz::DB::Base *db)
    {
        auto proto = worker->net_protocol();
        if (proto)
        {
            proto->send_answer(Cmd::GET_SCHEME, msg_id)
                    << uint8_t(ST_HASH_FLAG)
                    << proto->structure_sync().get_structure_hash_for_all(*db, DB::Schemed_Model::default_scheme_id());
        }
    });
}

void Structure_Synchronizer::send_structure(uint8_t struct_type, uint8_t msg_id)
{
    Worker* worker = protocol_->worker();
    db_thread()->add_query([worker, msg_id, struct_type](Helpz::DB::Base *db)
    {
        auto proto = worker->net_protocol();
        if (proto)
        {
            Helpz::Network::Protocol_Sender sender = proto->send_answer(Cmd::GET_SCHEME, msg_id);
            sender << struct_type;
//        sender.timeout(nullptr, std::chrono::minutes(5), std::chrono::seconds(90)); // Пока нет возможности отправлять ответ на ответ
            proto->structure_sync().add_structure_data(struct_type, sender, *db, DB::Schemed_Model::default_scheme_id());
        }
    });
}

void Structure_Synchronizer::send_modify_response(uint8_t struct_type, const QByteArray &buffer, uint32_t user_id)
{
    if (struct_type == ST_USER)
    {
        QString sql = get_scheme_group_users_fill_sql();
        db_thread()->add_pending_query(std::move(sql), std::vector<QVariantList>());
    }

    protocol_->send(Cmd::MODIFY_SCHEME)
            .timeout(nullptr, std::chrono::seconds(13))
            .writeRawData(buffer.constData(), buffer.size());

    QMetaObject::invokeMethod(protocol_->worker(), "restart_service_object", Qt::QueuedConnection, Q_ARG(uint32_t, user_id));
}

} // namespace Client
} // namespace Ver
} // namespace Das
