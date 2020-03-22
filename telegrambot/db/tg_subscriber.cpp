#include "tg_subscriber.h"

namespace Das {
namespace DB {

Tg_Subscriber::Tg_Subscriber(uint32_t id, int64_t chat_id, uint32_t group_id) :
    id_(id), group_id_(group_id), chat_id_(chat_id) {}

uint32_t Tg_Subscriber::id() const { return id_; }
void Tg_Subscriber::set_id(uint32_t id) { id_ = id; }

qint64 Tg_Subscriber::chat_id() const { return chat_id_; }
void Tg_Subscriber::set_chat_id(qint64 chat_id) { chat_id_ = chat_id; }

uint32_t Tg_Subscriber::group_id() const { return group_id_; }
void Tg_Subscriber::set_group_id(uint32_t group_id) { group_id_ = group_id; }


} // namespace DB
} // namespace Das
