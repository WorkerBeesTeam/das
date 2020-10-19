#ifndef DAS_DIG_STATUS_H
#define DAS_DIG_STATUS_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/base_type.h>
#include <Das/log/log_base_item.h>

namespace Das {
namespace DB {

#define DIG_STATUS_DB_META_ARGS \
    DB_A(timestamp_msecs), DB_AN(user_id), \
    DB_A(group_id), DB_A(status_id), DB_AT(args)

class DAS_LIBRARY_SHARED_EXPORT DIG_Status_Base : public Log_Base_Item
{
public:
    enum Status_Direction : uint8_t { SD_ADD = 1, SD_DEL = 2 };

    DIG_Status_Base(DIG_Status_Base&&) = default;
    DIG_Status_Base(const DIG_Status_Base&) = default;
    DIG_Status_Base& operator=(DIG_Status_Base&&) = default;
    DIG_Status_Base& operator=(const DIG_Status_Base&) = default;
    DIG_Status_Base(qint64 timestamp_msecs = 0, uint32_t user_id = 0, uint32_t group_id = 0,
               uint32_t status_id = 0, const QStringList& args = QStringList(), Status_Direction direction = SD_ADD);

    uint32_t group_id() const;
    void set_group_id(const uint32_t &group_id);

    uint32_t status_id() const;
    void set_status_id(const uint32_t &status_id);

    QStringList args() const;
    void set_args(const QStringList &args);
    QVariant args_to_db() const;
    void set_args_from_db(const QString& value);

    bool is_removed() const;
    uint8_t direction() const;
    void set_direction(uint8_t direction);

    bool operator <(const DIG_Status& o) const;
private:
    uint32_t group_id_, status_id_;
    QStringList args_;

    friend QDataStream& operator>>(QDataStream& ds, DIG_Status_Base& item);
};

QDataStream& operator>>(QDataStream& ds, DIG_Status_Base& item);
QDataStream& operator<<(QDataStream& ds, const DIG_Status_Base& item);

class DAS_LIBRARY_SHARED_EXPORT DIG_Status : public DIG_Status_Base, public ID_Type
{
    HELPZ_DB_META(DIG_Status, "dig_status", "gs", DB_A(id), DIG_STATUS_DB_META_ARGS, DB_A(scheme_id))
public:
    using DIG_Status_Base::DIG_Status_Base;
};

} // namespace DB

using DIG_Status = DB::DIG_Status;

} // namespace Das

#endif // DAS_DIG_STATUS_H
