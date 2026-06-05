#include "auth.h"

namespace Das {
namespace DB {

MaxBot_Auth::MaxBot_Auth(int32_t external_user_id, qint64 expired, const QString &token) :
    external_user_id_(external_user_id), expired_(expired), token_(token)
{
}

int32_t MaxBot_Auth::external_user_id() const { return external_user_id_; }
void MaxBot_Auth::set_external_user_id(int32_t id) { external_user_id_ = id; }

qint64 MaxBot_Auth::expired() const { return expired_; }
void MaxBot_Auth::set_expired(qint64 timestamp) { expired_ = timestamp; }

QString MaxBot_Auth::token() const { return token_; }
void MaxBot_Auth::set_token(const QString &token) { token_ = token; }

} // namespace DB
} // namespace Das
