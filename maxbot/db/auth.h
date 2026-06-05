#pragma once

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT MaxBot_Auth
{
    HELPZ_DB_META(MaxBot_Auth, "maxbot_auth", "mba", DB_A(external_user_id), DB_A(expired), DB_A(token))
public:
    MaxBot_Auth(int32_t external_user_id = 0, qint64 expired = 0, const QString& token = {});

    int32_t external_user_id() const;
    void set_external_user_id(int32_t id);

    qint64 expired() const;
    void set_expired(qint64 timestamp);

    QString token() const;
    void set_token(const QString& token);
private:
    int32_t external_user_id_;
    qint64 expired_;
    QString token_;
};

} // namespace DB

using MaxBot_Auth = DB::MaxBot_Auth;

} // namespace Das
