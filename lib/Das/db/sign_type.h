#ifndef DAS_SIGN_TYPE_H
#define DAS_SIGN_TYPE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Sign_Type : public Named_Type
{
    HELPZ_DB_META(Sign_Type, "sign_type", "st", DB_A(id), DB_A(name), DB_A(scheme_id))
public:
    using Named_Type::Named_Type;
};
typedef Named_Type_Manager<Sign_Type> Sign_Manager;

} // namespace DB

using Sign_Type = DB::Sign_Type;
using Sign_Manager = DB::Sign_Manager;

} // namespace Das

#endif // DAS_SIGN_TYPE_H
