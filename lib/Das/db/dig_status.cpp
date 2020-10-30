#include "dig_status.h"

namespace Das {
namespace DB {

DIG_Status_Base::DIG_Status_Base(qint64 timestamp_msecs, uint32_t user_id, uint32_t group_id,
                       uint32_t status_id, const QStringList &args, Status_Direction direction) :
    Log_Base_Item(timestamp_msecs, user_id, direction == SD_DEL),
    group_id_(group_id), status_id_(status_id), args_(args)
{
}

uint32_t DIG_Status_Base::group_id() const { return group_id_; }
void DIG_Status_Base::set_group_id(const uint32_t &group_id) { group_id_ = group_id; }

uint32_t DIG_Status_Base::status_id() const { return status_id_; }
void DIG_Status_Base::set_status_id(const uint32_t &status_id) { status_id_ = status_id; }

QStringList DIG_Status_Base::args() const { return args_; }
void DIG_Status_Base::set_args(const QStringList &args) { args_ = args; }

QVariant DIG_Status_Base::args_to_db() const
{
    if (args_.isEmpty())
        return QVariant();
    QStringList args{args_};
    for (QString& arg: args)
        arg = arg.remove('\n');
    return args.join('\n');
}

void DIG_Status_Base::set_args_from_db(const QString& value)
{
    if (!value.isEmpty())
        args_ = value.split('\n');
}

bool DIG_Status_Base::is_removed() const { return flag_; }

uint8_t DIG_Status_Base::direction() const { return flag_ ? SD_DEL : SD_ADD; }
void DIG_Status_Base::set_direction(uint8_t direction) { flag_ = direction == SD_DEL; }

bool DIG_Status_Base::operator <(const DIG_Status_Base &o) const
{
    return group_id() < o.group_id() || (group_id() == o.group_id() && status_id() < o.status_id());
}

QDataStream &operator>>(QDataStream &ds, DIG_Status_Base &item)
{
    return ds >> static_cast<Log_Base_Item&>(item) >> item.group_id_ >> item.status_id_ >> item.args_;
}

QDataStream &operator<<(QDataStream &ds, const DIG_Status_Base &item)
{
    return ds << static_cast<const Log_Base_Item&>(item) << item.group_id() << item.status_id() << item.args();
}

} // namespace DB
} // namespace Das
