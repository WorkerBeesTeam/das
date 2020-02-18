#ifndef DAS_SERVER_STRUCTURE_SYNCHRONIZER_2_1_H
#define DAS_SERVER_STRUCTURE_SYNCHRONIZER_2_1_H

#include <set>

#include <Das/type_managers.h>
#include <Das/device.h>
#include <Das/db/dig_status.h>

#include "database/db_scheme.h"
#include "base_synchronizer.h"
#include "structure_synchronizer_base_2_1.h"

namespace Das {
namespace Ver_2_1 {
namespace Server {

using namespace Das::Server;

class Structure_Synchronizer final : public Base_Synchronizer, public Structure_Synchronizer_Base
{
public:
    Structure_Synchronizer(Protocol_Base* protocol);

    void check(bool custom_only);

    void process_modify(uint32_t user_id, uint8_t struct_type, QIODevice* data_dev);

    void process_statuses(QVector<Ver_2_1_DIG_Status>&& ups_vect, QVector<Ver_2_1_DIG_Status>&& del_vect);

    bool need_to_use_parent_table(uint8_t struct_type) const;
    QString get_db_name(uint8_t struct_type) const;

private:
    void send_modify_response(uint8_t struct_type, const QByteArray &buffer, uint32_t user_id) override;

    Helpz::Network::Protocol_Sender send_scheme_request(uint8_t struct_type);
    void process_scheme(uint8_t struct_type, QIODevice* data_dev);

    void process_scheme_items_hash(QVector<QPair<uint32_t, uint16_t> >&& client_hash_vect_arg, uint8_t struct_type);
    void process_scheme_hash(const QByteArray& client_hash, uint8_t struct_type);
    void process_scheme_data(uint8_t struct_type, QIODevice* data_dev, bool delete_if_not_exist);
    bool remove_scheme_rows(Helpz::DB::Base& db, uint8_t struct_type, const QVector<uint32_t>& delete_vect);

    bool remove_statuses(Helpz::DB::Base& db, const QVector<uint32_t>& delete_vect);

    QString get_auth_user_suffix(const QString& user_id_str) const;
    QString get_suffix(uint8_t struct_type) override;

    template<class T>
    bool sync_table(QVector<T>&& items, const QString& db_name, bool delete_if_not_exist);

    bool struct_sync_timeout_;
    std::set<uint8_t> struct_wait_set_;
};

} // namespace Server
} // namespace Ver_2_1
} // namespace Das

#endif // DAS_SERVER_STRUCTURE_SYNCHRONIZER_2_1_H
