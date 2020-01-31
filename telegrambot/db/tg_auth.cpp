#include "tg_auth.h"

namespace Das {
namespace Database {

Tg_Auth::Tg_Auth(int32_t tg_user_id, qint64 expired, const QString &token) :
    tg_user_id_(tg_user_id), expired_(expired), token_(token)
{
}

int32_t Tg_Auth::tg_user_id() const { return tg_user_id_; }
void Tg_Auth::set_tg_user_id(int32_t id) { tg_user_id_ = id; }

qint64 Tg_Auth::expired() const { return expired_; }
void Tg_Auth::set_expired(qint64 timestamp) { expired_ = timestamp; }

QString Tg_Auth::token() const { return token_; }
void Tg_Auth::set_token(const QString &token) { token_ = token; }

} // namespace Database
} // namespace Das
