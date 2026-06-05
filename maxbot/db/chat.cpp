#include "chat.h"

namespace Das {
namespace DB {

MaxBot_Chat::MaxBot_Chat(qint64 id, qint64 admin_id) :
    admin_id_(admin_id), id_(id)
{
}

qint64 MaxBot_Chat::id() const { return id_; }
void MaxBot_Chat::set_id(qint64 id) { id_ = id; }

qint64 MaxBot_Chat::admin_id() const { return admin_id_; }
void MaxBot_Chat::set_admin_id(qint64 admin_id) { admin_id_ = admin_id; }

} // namespace DB
} // namespace Das
