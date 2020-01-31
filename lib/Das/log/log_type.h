#ifndef DAS_LOG_TYPE_H
#define DAS_LOG_TYPE_H

#include <QDataStream>

namespace Das {

QString log_table_name(uint8_t log_type, const QString& db_name = QString());

enum Log_Type : uint8_t {
    LOG_VALUE = 1,
    LOG_EVENT,
};

class Log_Type_Wrapper
{
public:
    Log_Type_Wrapper(uint8_t log_type);
    Log_Type_Wrapper() = default;
    Log_Type_Wrapper(Log_Type_Wrapper&&) = default;
    Log_Type_Wrapper(const Log_Type_Wrapper&) = default;
    Log_Type_Wrapper& operator=(Log_Type_Wrapper&&) = default;
    Log_Type_Wrapper& operator=(const Log_Type_Wrapper&) = default;

    bool is_valid() const;
    operator uint8_t() const;

    uint8_t value() const;
    void set_value(uint8_t log_type);

    QString to_string() const;
private:
    uint8_t log_type_;
    friend QDataStream &operator >>(QDataStream &ds, Log_Type_Wrapper &log_type);
};
QDataStream &operator >> (QDataStream& ds, Log_Type_Wrapper& log_type);
QDataStream &operator << (QDataStream& ds, const Log_Type_Wrapper& log_type);

} // namespace Das

#endif // DAS_LOG_TYPE_H
