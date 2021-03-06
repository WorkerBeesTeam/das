#ifndef DAS_DATABASE_TG_AUTH_H
#define DAS_DATABASE_TG_AUTH_H

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Tg_Auth
{
    HELPZ_DB_META(Tg_Auth, "tg_auth", "ta", DB_A(tg_user_id), DB_A(expired), DB_A(token))
public:
    Tg_Auth(int32_t tg_user_id = 0, qint64 expired = 0, const QString& token = {});

    int32_t tg_user_id() const;
    void set_tg_user_id(int32_t id);

    qint64 expired() const;
    void set_expired(qint64 timestamp);

    QString token() const;
    void set_token(const QString& token);
private:
    int32_t tg_user_id_;
    qint64 expired_;
    QString token_;
};

} // namespace DB

using Tg_Auth = DB::Tg_Auth;

} // namespace Das

#endif // DAS_DATABASE_TG_AUTH_H
