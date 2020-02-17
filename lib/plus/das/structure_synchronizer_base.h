#ifndef DAS_STRUCTURE_SYNCHRONIZER_BASE_H
#define DAS_STRUCTURE_SYNCHRONIZER_BASE_H

#include <future>

#include <QLoggingCategory>

#include <Helpz/db_base.h>
//#include <Helpz/db_delete_row.h>
#include <Helpz/db_thread.h>

//#include <Das/type_managers.h>
//#include <Das/device.h>
//#include <Das/device_item.h>
//#include <Das/section.h>
//#include <Das/db/user.h>
//#include <Das/db/auth_group.h>
//#include <Das/db/device_item_value.h>
//#include <Das/db/dig_param_value.h>
//#include <Das/db/dig_mode.h>

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(Struct_Log)
Q_DECLARE_LOGGING_CATEGORY(Struct_Detail_Log)

namespace Ver {

struct Uncheck_Foreign
{
    Uncheck_Foreign(Helpz::DB::Base* p);
    ~Uncheck_Foreign();
    Helpz::DB::Base* p_;
};

struct Update_Info
{
    QStringList changed_field_names_;
    QVariantList changed_fields_;
};

class Structure_Synchronizer_Base;
struct Bad_Fix
{
    virtual ~Bad_Fix() = default;
    virtual Structure_Synchronizer_Base* get_structure_synchronizer() = 0;
};

class Structure_Synchronizer_Base
{
public:
    Structure_Synchronizer_Base(Helpz::DB::Thread *db_thread);
    virtual ~Structure_Synchronizer_Base();

    bool modified() const;
    void set_modified(bool modified);

    void process_modify_message(uint32_t user_id, uint8_t struct_type, QIODevice* data_dev,
                                uint32_t scheme_id, std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix);

protected:
    QByteArray get_structure_hash(uint8_t struct_type, Helpz::DB::Base& db, uint32_t scheme_id);
    QByteArray get_structure_hash_for_all(Helpz::DB::Base& db, uint32_t scheme_id);

    Helpz::DB::Thread *db_thread() const;

    std::vector<uint8_t> get_main_table_types() const;
    bool is_main_table(uint8_t struct_type) const;

    virtual bool is_can_modify(uint8_t struct_type) const;
    virtual void fill_suffix(uint8_t struct_type, QString &where_str);

    void add_structure_data(uint8_t struct_type, QDataStream& ds, Helpz::DB::Base& db, uint32_t scheme_id);
    void add_structure_items_data(uint8_t struct_type, const QVector<uint32_t>& id_vect, QDataStream& ds, Helpz::DB::Base& db, uint32_t scheme_id);

    QMap<uint32_t, uint16_t> get_structure_hash_map_by_type(uint8_t struct_type, Helpz::DB::Base& db, uint32_t scheme_id);
private:
    template<typename T>
    QString get_db_list_suffix(uint8_t struct_type, const QVector<uint32_t>& id_vect, uint32_t scheme_id);

    template<typename T>
    QVector<T> get_db_list(uint8_t struct_type, Helpz::DB::Base& db, uint32_t scheme_id, const QVector<uint32_t>& id_vect);

    void add_structure_template(uint8_t struct_type, QDataStream& ds, Helpz::DB::Base& db, uint32_t scheme_id, const QVector<uint32_t>& id_vect = {});
    template<typename T>
    QMap<uint32_t, uint16_t> get_structure_hash_map(uint8_t struct_type, Helpz::DB::Base& db, uint32_t scheme_id);

    virtual void send_modify_response(uint8_t struct_type, const QByteArray &buffer, uint32_t user_id) = 0;

    template<typename T>
    void modify(QVector<T>&& upd_vect, QVector<T>&& insrt_vect, QVector<uint32_t>&& del_vect, uint32_t user_id,
                uint8_t struct_type, uint32_t scheme_id, std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix);

    template<class T>
    bool modify_table(uint8_t struct_type, Helpz::DB::Base& db,
                      const QVector<T>& update_vect,
                      QVector<T>& insert_vect,
                      const QVector<uint32_t>& delete_vect, uint32_t scheme_id);

private:
    bool modified_;

    Helpz::DB::Thread *db_thread_;
};

} // namespace Ver
} // namespace Das

#endif // DAS_STRUCTURE_SYNCHRONIZER_BASE_H
