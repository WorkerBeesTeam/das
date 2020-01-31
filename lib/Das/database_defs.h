#ifndef DAS_DATABASE_DEFS_H
#define DAS_DATABASE_DEFS_H

#include <type_traits>

#include <QVariant>

namespace Das {
namespace Database {

template<typename T, typename FT, typename AT>
void db_set_value_from_variant(T& obj, void (FT::*setter)(AT), const QVariant& value)
{
    using VALUE_TYPE = typename std::decay<AT>::type;
    (obj.*setter)(value.value<VALUE_TYPE>());
}

template<typename T, typename FT, typename AT>
void db_set_value_from_variant(T& obj, AT FT::*member_value, const QVariant& value)
{
    using VALUE_TYPE = typename std::decay<AT>::type;
    (obj.*member_value) = value.value<VALUE_TYPE>();
}

} // namespace Das
} // namespace Database

#endif // DAS_DATABASE_DEFS_H
