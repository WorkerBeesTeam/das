#ifndef DAS_DATABASE_TG_SUBSCRIBER_H
#define DAS_DATABASE_TG_SUBSCRIBER_H

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT Tg_Subscriber
{
    HELPZ_DB_META(Tg_Subscriber, "das_tg_subscriber", "ts", 3, DB_A(id), DB_A(chat_id), DB_A(group_id))
public:
    Tg_Subscriber(uint32_t id = 0, int64_t chat_id = 0, uint32_t group_id = 0);

    uint32_t id() const;
    void set_id(uint32_t id);

    qint64 chat_id() const;
    void set_chat_id(qint64 chat_id);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);

private:
    uint32_t id_, group_id_;
    qint64 chat_id_;
};

} // namespace Database

using Tg_Subscriber = Database::Tg_Subscriber;

} // namespace Das

#endif // DAS_DATABASE_TG_SUBSCRIBER_H
