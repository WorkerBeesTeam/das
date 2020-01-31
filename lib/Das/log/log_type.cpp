#include <Helpz/db_table.h>

#include "log_pack.h"
#include "log_type.h"

namespace Das {

QString log_table_name(uint8_t log_type, const QString& db_name)
{
    switch (static_cast<Log_Type>(log_type))
    {
    case LOG_VALUE: return Helpz::Database::db_table_name<Log_Value_Item>(db_name);
    case LOG_EVENT: return Helpz::Database::db_table_name<Log_Event_Item>(db_name);
    default: break;
    }
    return {};
}

Log_Type_Wrapper::Log_Type_Wrapper(uint8_t log_type) :
    log_type_(log_type)
{
}

bool Log_Type_Wrapper::is_valid() const
{
    return log_type_ == LOG_VALUE || log_type_ == LOG_EVENT;
}

Log_Type_Wrapper::operator uint8_t() const
{
    return log_type_;
}

uint8_t Log_Type_Wrapper::value() const
{
    return log_type_;
}

void Log_Type_Wrapper::set_value(uint8_t log_type)
{
    log_type_ = log_type;
}

QString Log_Type_Wrapper::to_string() const
{
    switch (static_cast<Log_Type>(log_type_))
    {
    case LOG_VALUE: return "[Value]";
    case LOG_EVENT: return "[Event]";
    default: break;
    }
    return {};
}

QDataStream &operator >>(QDataStream &ds, Log_Type_Wrapper &log_type)
{
    return ds >> log_type.log_type_;
}

QDataStream &operator <<(QDataStream &ds, const Log_Type_Wrapper &log_type)
{
    return ds << log_type.value();
}

} // namespace Das
