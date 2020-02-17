#include <Das/commands.h>

#include "worker.h"
#include "worker_structure_synchronizer.h"

namespace Das {

Worker_Structure_Synchronizer::Worker_Structure_Synchronizer(Worker* worker) :
    Ver::Structure_Synchronizer_Base(worker->db_pending()),
    worker_(worker)
{
}

void Worker_Structure_Synchronizer::send_modify_response(uint8_t struct_type, const QByteArray& buffer, uint32_t user_id)
{
    if (struct_type == Ver::ST_USER)
    {
        QString sql = Ver::Client::Structure_Synchronizer::get_scheme_group_users_fill_sql();
        db_thread()->add_pending_query(std::move(sql), std::vector<QVariantList>());
    }

    auto proto = worker_->net_protocol();
    if (proto)
    {
        proto->send(Ver::Cmd::MODIFY_SCHEME).writeRawData(buffer.constData(), buffer.size());
    }

    worker_->restart_service_object(user_id);
}

} // namespace Das
