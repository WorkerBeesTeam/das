#pragma once

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT MaxBot_Chat
{
    HELPZ_DB_META(MaxBot_Chat, "maxbot_chat", "mbc", DB_A(id), DB_AN(admin_id))
public:
    MaxBot_Chat(qint64 id = 0, qint64 admin_id = 0);

    qint64 id() const;
    void set_id(qint64 id);

    qint64 admin_id() const;
    void set_admin_id(qint64 admin_id);
private:
    qint64 admin_id_;
    qint64 id_;
};

} // namespace DB
} // namespace Das
