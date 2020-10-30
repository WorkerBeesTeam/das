#ifndef DAS_DATABASE_DIG_MODE_H
#define DAS_DATABASE_DIG_MODE_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/base_type.h>
#include <Das/log/log_base_item.h>

namespace Das {
namespace DB {

#define DIG_MODE_DB_META_ARGS \
    DB_A(timestamp_msecs), DB_AN(user_id), DB_A(group_id), DB_A(mode_id), DB_A(scheme_id)

class DAS_LIBRARY_SHARED_EXPORT DIG_Mode_Base : public Log_Base_Item
{
public:
    explicit DIG_Mode_Base(qint64 timestamp_msecs = 0, uint32_t user_id = 0, uint32_t group_id = 0, uint32_t mode_id = 0);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);

    uint32_t mode_id() const;
    void set_mode_id(uint32_t mode_id);

private:
    uint32_t group_id_, mode_id_;

    friend QDataStream &operator>>(QDataStream &ds, DIG_Mode_Base& item);
};

QDataStream &operator>>(QDataStream &ds, DIG_Mode_Base& item);
QDataStream &operator<<(QDataStream &ds, const DIG_Mode_Base& item);

class DAS_LIBRARY_SHARED_EXPORT DIG_Mode : public DIG_Mode_Base, public ID_Type
{
    HELPZ_DB_META(DIG_Mode, "dig_mode", "gm", DB_A(id), DIG_MODE_DB_META_ARGS)
public:
    using DIG_Mode_Base::DIG_Mode_Base;
    using DIG_Mode_Base::operator =;
    explicit DIG_Mode(const DIG_Mode_Base& o) : DIG_Mode_Base{o} {}
    explicit DIG_Mode(DIG_Mode_Base&& o) : DIG_Mode_Base{std::move(o)} {}
};

} // namespace DB

using DIG_Mode = DB::DIG_Mode;

} // namespace Das

#endif // DAS_DATABASE_DIG_MODE_H
