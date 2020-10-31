#ifndef DAS_DATABASE_DIG_PARAM_TYPE_H
#define DAS_DATABASE_DIG_PARAM_TYPE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace DB {

struct DAS_LIBRARY_SHARED_EXPORT DIG_Param_Type : public QObject, public Titled_Type {
private:
    Q_OBJECT
    Q_PROPERTY(uint32_t id READ id WRITE set_id)
    Q_PROPERTY(QString name READ name WRITE set_name)
    Q_PROPERTY(QString title READ title WRITE set_title)
    Q_PROPERTY(QString description READ description WRITE set_description)
    Q_PROPERTY(Value_Type value_type READ value_type WRITE set_value_type)
    Q_PROPERTY(Value_Type type READ value_type WRITE set_value_type)

    HELPZ_DB_META(DIG_Param_Type, "dig_param_type", "gpt", DB_A(id), DB_A(name), DB_A(title),
                  DB_ANS(description), DB_A(value_type), DB_AM(group_type_id), DB_AMN(parent_id), DB_A(scheme_id))
public:
    enum Value_Type : uint8_t
    {
        VT_UNKNOWN = 0,
        VT_INT,
        VT_BOOL,
        VT_FLOAT,
        VT_STRING,
        VT_BYTES,
        VT_TIME,
        VT_RANGE,
        VT_COMBO,
    };
    Q_ENUM(Value_Type)

    DIG_Param_Type(uint id = 0, const QString& name = QString(), const QString& title = QString(), QString description = QString(),
              Value_Type value_type = VT_UNKNOWN,
              uint group_type_id = 0, uint parent_id = 0);
    DIG_Param_Type(const DIG_Param_Type& o);
    DIG_Param_Type(DIG_Param_Type&& o);
    DIG_Param_Type& operator=(const DIG_Param_Type& o);
    DIG_Param_Type& operator=(DIG_Param_Type&& o);

    QString description() const;
    void set_description(const QString &description);

    Value_Type value_type() const;
    void set_value_type(const Value_Type &value_type);

    uint32_t group_type_id = 0;
    uint32_t parent_id = 0;

    Value_Type value_type_ = VT_UNKNOWN;
    QString description_;
};

QDataStream& operator<<(QDataStream& ds, const DIG_Param_Type& param);
QDataStream& operator>>(QDataStream& ds, DIG_Param_Type& param);

struct DAS_LIBRARY_SHARED_EXPORT DIG_Param_Type_Manager : public Titled_Type_Manager<DIG_Param_Type> {};

} // namespace DB

using DIG_Param_Type = DB::DIG_Param_Type;
using DIG_Param_Type_Manager = DB::DIG_Param_Type_Manager;

} // namespace Das

#endif // DAS_DATABASE_DIG_PARAM_TYPE_H
