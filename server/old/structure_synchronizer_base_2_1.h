#ifndef DAS_STRUCTURE_SYNCHRONIZER_BASE_2_1_H
#define DAS_STRUCTURE_SYNCHRONIZER_BASE_2_1_H

#include <future>

#include <QLoggingCategory>

#include <Helpz/db_base.h>
#include <Helpz/db_delete_row.h>
#include <Helpz/db_thread.h>

#include <Das/type_managers.h>
#include <Das/device.h>
#include <Das/device_item.h>
#include <Das/section.h>

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(Struct_Log)

namespace Ver_2_1 {

class DAS_LIBRARY_SHARED_EXPORT Ver_2_1_DIG_Status : public DIG_Status {};

QDataStream& operator>>(QDataStream& ds, Ver_2_1_DIG_Status& item);
QDataStream& operator<<(QDataStream& ds, const Ver_2_1_DIG_Status& item);

class Structure_Synchronizer_Base;
struct Bad_Fix
{
    virtual ~Bad_Fix() = default;
    virtual Structure_Synchronizer_Base* get_structure_synchronizer() = 0;
};

class Structure_Synchronizer_Base
{
public:
    Structure_Synchronizer_Base(Helpz::Database::Thread *db_thread);
    virtual ~Structure_Synchronizer_Base();

    bool modified() const;
    void set_modified(bool modified);

    QString common_db_name() const;
    void set_common_db_name(const QString &common_db_name);

    void process_modify_message(uint32_t user_id, uint8_t struct_type, QIODevice* data_dev,
                                const QString& db_name = QString(), std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix = nullptr);

protected:
    QByteArray get_structure_hash(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name = QString());
    QByteArray get_structure_hash_for_all(Helpz::Database::Base& db, const QString& db_name = QString());

    Helpz::Database::Thread *db_thread() const;

    std::vector<uint8_t> get_main_table_types() const;
    bool is_main_table(uint8_t struct_type) const;

    virtual QString get_suffix(uint8_t struct_type);

    void add_structure_data(uint8_t struct_type, QDataStream& ds, Helpz::Database::Base& db, const QString& db_name = QString());
    void add_structure_items_data(uint8_t struct_type, const QVector<uint32_t>& id_vect, QDataStream& ds, Helpz::Database::Base& db, const QString& db_name = QString());

    QVector<QPair<uint32_t, uint16_t>> get_structure_hash_vect_by_type(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name = QString());
private:
    template<typename T>
    QString get_db_list_suffix(uint8_t struct_type, const QVector<uint32_t>& id_vect);

    template<typename T>
    QVector<T> get_db_list(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name, const QVector<uint32_t>& id_vect);

    void add_structure_template(uint8_t struct_type, QDataStream& ds, Helpz::Database::Base& db, const QString& db_name = QString(), const QVector<uint32_t>& id_vect = {});
    template<typename T>
    QVector<QPair<uint32_t, uint16_t>> get_structure_hash_vect(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name = QString());

    virtual void send_modify_response(uint8_t struct_type, const QByteArray &buffer, uint32_t user_id) = 0;

    template<typename T>
    void modify(QVector<T>&& upd_vect, QVector<T>&& insrt_vect, QVector<uint32_t>&& del_vect, uint32_t user_id,
                uint8_t struct_type, const QString& db_name = QString(), std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix = nullptr);

    template<class T>
    bool modify_table(uint8_t struct_type, Helpz::Database::Base& db,
                      const QVector<T>& update_vect,
                      QVector<T>& insert_vect,
                      const QVector<uint32_t>& delete_vect,
                      const QString& db_name = QString());

private:
    bool modified_;
    QString common_db_name_;

    Helpz::Database::Thread *db_thread_;
};

} // namespace Ver_2_1
} // namespace Das

#endif // DAS_STRUCTURE_SYNCHRONIZER_BASE_2_1_H
