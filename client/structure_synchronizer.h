#ifndef DAS_CLIENT_STRUCTURE_SYNCHRONIZER_H
#define DAS_CLIENT_STRUCTURE_SYNCHRONIZER_H

#include <Helpz/net_protocol.h>

#include <Das/scheme.h>
#include <Das/db/dig_status.h>
#include <plus/das/structure_synchronizer_base.h>

namespace Das {
namespace Ver {
namespace Client {

//using namespace Das::Client;

class Protocol;

class Structure_Synchronizer : public Structure_Synchronizer_Base
{
public:
    Structure_Synchronizer(Helpz::DB::Thread *db_thread, Protocol* protocol);

    static QString get_scheme_group_users_fill_sql();

    void send_scheme_structure(uint8_t struct_type, uint8_t msg_id, QIODevice* data_dev);
private:
    static Helpz::Net::Protocol_Sender send_answer(Ver::Client::Protocol& proto, uint8_t struct_type, uint8_t msg_id);

    void send_structure_items_hash(uint8_t struct_type, uint8_t msg_id);
    void add_structure_items_hash(uint8_t struct_type, uint8_t msg_id, Helpz::DB::Base& db);

    void send_structure_items(const QVector<uint32_t>& id_vect, uint8_t struct_type, uint8_t msg_id);

    void send_structure_hash(uint8_t struct_type, uint8_t msg_id);
    void send_structure_hash_for_all(uint8_t msg_id);
    void send_structure(uint8_t struct_type, uint8_t msg_id);

    void send_modify_response(uint8_t struct_type, const QByteArray &buffer, uint32_t user_id) override;

    Protocol* protocol_;

    std::shared_ptr<std::atomic<std::chrono::system_clock::time_point>> wait_restart_from_;
};

} // namespace Client
} // namespace Ver
} // namespace Das

#endif // DAS_CLIENT_STRUCTURE_SYNCHRONIZER_H
