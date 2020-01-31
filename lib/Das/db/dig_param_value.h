#ifndef DAS_DATABASE_DIG_PARAM_VALUE_H
#define DAS_DATABASE_DIG_PARAM_VALUE_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT DIG_Param_Value : public Schemed_Model
{
    HELPZ_DB_META(DIG_Param_Value, "dig_param_value", "gpv", 4, DB_A(id), DB_A(group_param_id), DB_A(value), DB_A(scheme_id))
public:
    DIG_Param_Value(uint32_t group_param_id = 0, const QString& value = QString());

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t group_param_id() const;
    void set_group_param_id(uint32_t group_param_id);

    QString value() const;
    void set_value(const QString &value);

private:
    uint32_t id_, group_param_id_;
    QString value_;

    friend QDataStream& operator>>(QDataStream& ds, DIG_Param_Value& item);
};

QDataStream& operator<<(QDataStream& ds, const DIG_Param_Value& item);
QDataStream& operator>>(QDataStream& ds, DIG_Param_Value& item);

} // namespace Database

using DIG_Param_Value = Database::DIG_Param_Value;

} // namespace Das

#endif // DAS_DATABASE_DIG_PARAM_VALUE_H
