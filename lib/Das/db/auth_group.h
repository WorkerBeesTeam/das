#ifndef DAS_DATABASE_AUTH_GROUP_H
#define DAS_DATABASE_AUTH_GROUP_H

#include <Helpz/db_meta.h>

#include "base_type.h"

namespace Das {
namespace DB {
class Auth_Group;
class Auth_Group_Permission;
} // namespace DB
} // namespace Das
namespace Helpz {
namespace DB {
template<> inline bool is_table_use_common_prefix<Das::DB::Auth_Group>() { return false; }
template<> inline bool is_table_use_common_prefix<Das::DB::Auth_Group_Permission>() { return false; }
} // namespace DB
} // namespace Helpz

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Auth_Group : public Base_Type
{
    HELPZ_DB_META(Auth_Group, "auth_group", "ag", 2, DB_A(id), DB_A(name))
public:
    using Base_Type::Base_Type;
};

// ----------------------------------------------------------------------------------------------------

class DAS_LIBRARY_SHARED_EXPORT Auth_Group_Permission
{
    HELPZ_DB_META(Auth_Group_Permission, "auth_group_permissions", "agp", 3, DB_A(id), DB_A(group_id), DB_A(permission_id))
public:
    Auth_Group_Permission(uint32_t id = 0, uint32_t group_id = 0, uint32_t permission_id = 0);

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);

    uint32_t permission_id() const;
    void set_permission_id(uint32_t permission_id);

private:
    uint32_t id_, group_id_, permission_id_;

    friend QDataStream& operator>>(QDataStream& ds, Auth_Group_Permission& item);
};

QDataStream& operator<<(QDataStream& ds, const Auth_Group_Permission& item);
QDataStream& operator>>(QDataStream& ds, Auth_Group_Permission& item);

} // namespace DB

using Auth_Group = DB::Auth_Group;
using Auth_Group_Permission = DB::Auth_Group_Permission;

} // namespace Das

#endif // DAS_DATABASE_AUTH_GROUP_H
