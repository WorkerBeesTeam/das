#pragma once

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT MaxBot_Subscriber
{
    HELPZ_DB_META(MaxBot_Subscriber, "maxbot_subscriber", "mbs", DB_A(id), DB_A(chat_id), DB_A(group_id))
public:
    MaxBot_Subscriber(uint32_t id = 0, int64_t chat_id = 0, uint32_t group_id = 0);

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

} // namespace DB

using MaxBot_Subscriber = DB::MaxBot_Subscriber;

} // namespace Das
