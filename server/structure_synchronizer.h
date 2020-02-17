#ifndef DAS_SERVER_STRUCTURE_SYNCHRONIZER_H
#define DAS_SERVER_STRUCTURE_SYNCHRONIZER_H

#include <set>

#include <Das/type_managers.h>
#include <Das/device.h>
#include <Das/db/dig_status.h>
#include <plus/das/structure_synchronizer_base.h>

#include "database/db_scheme.h"
#include "base_synchronizer.h"

namespace Das {
namespace Ver {
namespace Server {

using namespace Das::Server;

class Structure_Synchronizer final : public Base_Synchronizer, public Structure_Synchronizer_Base
{
public:
    Structure_Synchronizer(Protocol_Base* protocol);
    ~Structure_Synchronizer();

    void check(bool custom_only, bool force_clean = true);
    void process_modify(uint32_t user_id, uint8_t struct_type, QIODevice* data_dev);

    bool need_to_use_parent_table(uint8_t struct_type) const;

    void check_values();
    void check_statuses();

    QVector<Device_Item_Value> get_devitem_values() const;
    std::set<DIG_Status> get_statuses();

    void change_devitem_value(const Device_Item_Value& value);
    void change_status(const DIG_Status& item);
    QVector<DIG_Status> insert_statuses(const QVector<DIG_Status> &statuses);
private:
    void change_devitem_value_no_block(const Device_Item_Value& value);
    static bool devitem_value_compare(const Device_Item_Value& value1, const Device_Item_Value& value2);
    void insert_devitem_values(QVector<Device_Item_Value> &&value_vect);
    bool clear_devitem_values_with_save();
    static void save_devitem_values(Helpz::DB::Base* db, const QVector<Device_Item_Value>& value_vect, uint32_t scheme_id);
    void clear_devitem_values();
    void clear_statuses();

    void set_synchronized(uint8_t struct_type);

    void send_modify_response(uint8_t struct_type, const QByteArray &buffer, uint32_t user_id) override;

    Helpz::Network::Protocol_Sender send_scheme_request(uint8_t struct_type);
    void process_scheme(uint8_t struct_type, QIODevice* data_dev);

    void process_scheme_items_hash(QMap<uint32_t, uint16_t> &&client_hash_map_arg, uint8_t struct_type);
    void process_scheme_hash(const QByteArray& client_hash, uint8_t struct_type);
    void process_scheme_data(uint8_t struct_type, QIODevice* data_dev, bool delete_if_not_exist);
    bool remove_scheme_rows(Helpz::DB::Base& db, uint8_t struct_type, const QVector<uint32_t>& delete_vect);

    QString get_auth_user_suffix(const QString& user_id_str) const;
    void fill_suffix(uint8_t struct_type, QString& where_str) override;

    bool is_can_modify(uint8_t struct_type) const override;

    template<class T>
    bool sync_table(QVector<T>&& items, uint32_t scheme_id, bool delete_if_not_exist);

    bool struct_sync_timeout_;
    std::set<uint8_t> struct_wait_set_, synchronized_;

    mutable std::mutex data_mutex_;
    QVector<Device_Item_Value> devitem_value_vect_;
    std::set<DIG_Status> status_set_;
};

} // namespace Server
} // namespace Ver
} // namespace Das

#endif // DAS_SERVER_STRUCTURE_SYNCHRONIZER_H
