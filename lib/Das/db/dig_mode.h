#ifndef DAS_DATABASE_DIG_MODE_H
#define DAS_DATABASE_DIG_MODE_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/log/log_base_item.h>

namespace Das {
namespace DB {

#define DIG_MODE_DB_META(x, y, z) \
    HELPZ_DB_META(x, y, z, 6, DB_A(id), DB_A(timestamp_msecs), \
        DB_AN(user_id), DB_A(group_id), DB_A(mode_id), DB_A(scheme_id))

class DAS_LIBRARY_SHARED_EXPORT DIG_Mode : public Log_Base_Item
{
    DIG_MODE_DB_META(DIG_Mode, "dig_mode", "gm")
public:
    DIG_Mode(qint64 timestamp_msecs = 0, uint32_t user_id = 0, uint32_t group_id = 0, uint32_t mode_id = 0);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);

    uint32_t mode_id() const;
    void set_mode_id(uint32_t mode_id);

private:
    uint32_t group_id_, mode_id_;

    friend QDataStream &operator>>(QDataStream &ds, DIG_Mode& item);
};

QDataStream &operator>>(QDataStream &ds, DIG_Mode& item);
QDataStream &operator<<(QDataStream &ds, const DIG_Mode& item);

} // namespace DB

using DIG_Mode = DB::DIG_Mode;

} // namespace Das

#endif // DAS_DATABASE_DIG_MODE_H
