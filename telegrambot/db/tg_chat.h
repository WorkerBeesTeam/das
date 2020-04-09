#ifndef DAS_DB_TG_CHAT_H
#define DAS_DB_TG_CHAT_H

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Tg_Chat
{
    HELPZ_DB_META(Tg_Chat, "tg_chat", "tc", 2, DB_A(id), DB_AN(admin_id))
public:
    Tg_Chat(qint64 id = 0, int32_t admin_id = 0);

    qint64 id() const;
    void set_id(qint64 id);

    int32_t admin_id() const;
    void set_admin_id(int32_t admin_id);
private:
    int32_t admin_id_;
    qint64 id_;
};

} // namespace DB
} // namespace Das

#endif // DAS_DB_TG_CHAT_H
