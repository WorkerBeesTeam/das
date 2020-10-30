#ifndef DAS_DATABASE_DIG_PARAM_VALUE_H
#define DAS_DATABASE_DIG_PARAM_VALUE_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/base_type.h>
#include <Das/log/log_base_item.h>

namespace Das {
namespace DB {

class DIG_Param_Value_Base
{
public:
    explicit DIG_Param_Value_Base(uint32_t group_param_id = 0, const QString& value = QString());

    uint32_t group_param_id() const;
    void set_group_param_id(uint32_t group_param_id);

    QString value() const;
    void set_value(const QString &value);

private:
    uint32_t group_param_id_;
    QString value_;

    friend QDataStream& operator>>(QDataStream& ds, DIG_Param_Value_Base& item);
};

QDataStream& operator<<(QDataStream& ds, const DIG_Param_Value_Base& item);
QDataStream& operator>>(QDataStream& ds, DIG_Param_Value_Base& item);

#define DIG_PARAM_VALUE_DB_META_ARGS \
    DB_A(timestamp_msecs), DB_AN(user_id), DB_A(group_param_id), DB_A(value), DB_A(scheme_id)

class DAS_LIBRARY_SHARED_EXPORT DIG_Param_Value_Base2 : public Log_Base_Item, public DIG_Param_Value_Base
{
public:
    explicit DIG_Param_Value_Base2(qint64 timestamp_msecs = 0, uint32_t user_id = 0,
                                   uint32_t group_param_id = 0, const QString& value = QString());
};

QDataStream& operator<<(QDataStream& ds, const DIG_Param_Value_Base2& item);
QDataStream& operator>>(QDataStream& ds, DIG_Param_Value_Base2& item);

class DAS_LIBRARY_SHARED_EXPORT DIG_Param_Value : public DIG_Param_Value_Base2, public ID_Type
{
    HELPZ_DB_META(DIG_Param_Value, "dig_param_value", "gpv", DB_A(id), DIG_PARAM_VALUE_DB_META_ARGS)
public:
    using DIG_Param_Value_Base2::DIG_Param_Value_Base2;
    using DIG_Param_Value_Base2::operator =;
    explicit DIG_Param_Value(const DIG_Param_Value_Base2& o) : DIG_Param_Value_Base2{o} {}
    explicit DIG_Param_Value(DIG_Param_Value_Base2&& o) : DIG_Param_Value_Base2{std::move(o)} {}
};

} // namespace DB

using DIG_Param_Value = DB::DIG_Param_Value;

} // namespace Das

#endif // DAS_DATABASE_DIG_PARAM_VALUE_H
