#ifndef DAS_DB_DISABLED_STATUS_H
#define DAS_DB_DISABLED_STATUS_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Disabled_Status : public Schemed_Model
{
    HELPZ_DB_META(Disabled_Status, "disabled_status", "ds", 4, DB_A(id), DB_A(group_id), DB_A(status_id), DB_A(scheme_id))
public:
    Disabled_Status(uint32_t id = 0, uint32_t group_id = 0, uint32_t status_id = 0);

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);

    uint32_t status_id() const;
    void set_status_id(uint32_t status_id);
private:
    uint32_t id_, group_id_, status_id_;

    friend QDataStream& operator>>(QDataStream& ds, Disabled_Status& item);
};

QDataStream& operator<<(QDataStream& ds, const Disabled_Status& item);
QDataStream& operator>>(QDataStream& ds, Disabled_Status& item);

} // namespace DB
} // namespace Das

#endif // DAS_DB_DISABLED_STATUS_H
