#include "subscriber.h"

namespace Das {
namespace DB {

MaxBot_Subscriber::MaxBot_Subscriber(uint32_t id, int64_t chat_id, uint32_t group_id) :
    id_(id), group_id_(group_id), chat_id_(chat_id) {}

uint32_t MaxBot_Subscriber::id() const { return id_; }
void MaxBot_Subscriber::set_id(uint32_t id) { id_ = id; }

qint64 MaxBot_Subscriber::chat_id() const { return chat_id_; }
void MaxBot_Subscriber::set_chat_id(qint64 chat_id) { chat_id_ = chat_id; }

uint32_t MaxBot_Subscriber::group_id() const { return group_id_; }
void MaxBot_Subscriber::set_group_id(uint32_t group_id) { group_id_ = group_id; }


} // namespace DB
} // namespace Das
