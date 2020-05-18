#include "tg_chat.h"

namespace Das {
namespace DB {

Tg_Chat::Tg_Chat(qint64 id, int32_t admin_id) :
    admin_id_(admin_id), id_(id)
{
}

qint64 Tg_Chat::id() const { return id_; }
void Tg_Chat::set_id(qint64 id) { id_ = id; }

int32_t Tg_Chat::admin_id() const { return admin_id_; }
void Tg_Chat::set_admin_id(int32_t admin_id) { admin_id_ = admin_id; }

} // namespace DB
} // namespace Das
