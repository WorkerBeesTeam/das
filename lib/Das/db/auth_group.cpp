#include "auth_group.h"

namespace Das {
namespace Database {

Auth_Group_Permission::Auth_Group_Permission(uint32_t id, uint32_t group_id, uint32_t permission_id) :
    id_(id), group_id_(group_id), permission_id_(permission_id)
{
}

uint32_t Auth_Group_Permission::id() const
{
    return id_;
}

void Auth_Group_Permission::set_id(uint32_t id)
{
    id_ = id;
}

uint32_t Auth_Group_Permission::group_id() const
{
    return group_id_;
}

void Auth_Group_Permission::set_group_id(uint32_t group_id)
{
    group_id_ = group_id;
}

uint32_t Auth_Group_Permission::permission_id() const
{
    return permission_id_;
}

void Auth_Group_Permission::set_permission_id(uint32_t permission_id)
{
    permission_id_ = permission_id;
}

QDataStream &operator<<(QDataStream &ds, const Auth_Group_Permission &item)
{
    return ds << item.id() << item.group_id() << item.permission_id();
}

QDataStream &operator>>(QDataStream &ds, Auth_Group_Permission &item)
{
    return ds >> item.id_ >> item.group_id_ >> item.permission_id_;
}

} // namespace Database
} // namespace Das
