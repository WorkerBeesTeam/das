#ifndef DAS_DIG_STATUS_H
#define DAS_DIG_STATUS_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT DIG_Status : public Schemed_Model
{
    HELPZ_DB_META(DIG_Status, "dig_status", "gs", 5, DB_A(id), DB_A(group_id), DB_A(status_id), DB_AT(args), DB_A(scheme_id))
public:
    DIG_Status(uint32_t id = 0, uint32_t group_id = 0, uint32_t status_id = 0, const QStringList& args = QStringList());
    uint32_t id() const;
    void set_id(const uint32_t &id);

    uint32_t group_id() const;
    void set_group_id(const uint32_t &group_id);

    uint32_t status_id() const;
    void set_status_id(const uint32_t &status_id);

    QStringList args() const;
    void set_args(const QStringList &args);
    QVariant args_to_db() const;
    void set_args_from_db(const QString& value);

    bool operator <(const DIG_Status& o) const;
private:
    uint32_t id_;
    uint32_t group_id_;
    uint32_t status_id_;
    QStringList args_;

    friend QDataStream& operator>>(QDataStream& ds, DIG_Status& item);
};

QDataStream& operator>>(QDataStream& ds, DIG_Status& item);
QDataStream& operator<<(QDataStream& ds, const DIG_Status& item);

} // namespace Database

using DIG_Status = Database::DIG_Status;

} // namespace Das

#endif // DAS_DIG_STATUS_H
