#ifndef DAS_SIGN_TYPE_H
#define DAS_SIGN_TYPE_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT Sign_Type : public Base_Type
{
    HELPZ_DB_META(Sign_Type, "sign_type", "st", 3, DB_A(id), DB_A(name), DB_A(scheme_id))
public:
    using Base_Type::Base_Type;
};
typedef Base_Type_Manager<Sign_Type> Sign_Manager;

} // namespace Database

using Sign_Type = Database::Sign_Type;
using Sign_Manager = Database::Sign_Manager;

} // namespace Das

#endif // DAS_SIGN_TYPE_H
