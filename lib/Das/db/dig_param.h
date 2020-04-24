#ifndef DAS_DATABASE_DIG_PARAM_H
#define DAS_DATABASE_DIG_PARAM_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT DIG_Param : public Schemed_Model
{
    HELPZ_DB_META(DIG_Param, "dig_param", "hgp", DB_A(id), DB_A(param_id), DB_A(group_id), DB_AN(parent_id), DB_A(scheme_id))
public:
    DIG_Param(uint32_t id = 0, uint32_t param_id = 0, uint32_t group_id = 0, uint32_t parent_id = 0);

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t param_id() const;
    void set_param_id(const uint32_t &param_id);

    uint32_t group_id() const;
    void set_group_id(const uint32_t &group_id);

    uint32_t parent_id() const;
    void set_parent_id(const uint32_t &parent_id);

private:
    uint32_t id_, param_id_, group_id_, parent_id_;

    friend QDataStream& operator>>(QDataStream& ds, DIG_Param& item);
};

QDataStream& operator<<(QDataStream& ds, const DIG_Param& item);
QDataStream& operator>>(QDataStream& ds, DIG_Param& item);

} // namespace DB

using DIG_Param = DB::DIG_Param;

} // namespace Das

#endif // DAS_DATABASE_DIG_PARAM_H
