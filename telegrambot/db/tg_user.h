#ifndef DAS_DATABASE_TG_USER_H
#define DAS_DATABASE_TG_USER_H

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Tg_User
{
    HELPZ_DB_META(Tg_User, "tg_user", "tu", 7, DB_A(id), DB_AN(user_id), DB_A(first_name),
                  DB_A(last_name), DB_A(user_name), DB_A(lang), DB_AN(private_chat_id))
public:
    Tg_User(int32_t id = 0, uint32_t user_id = 0, const QString& first_name = {},
            const QString& last_name = {}, const QString& user_name = {}, const QString& lang = {},
            qint64 private_chat_id = 0);

    int32_t id() const;
    void set_id(int32_t id);

    uint32_t user_id() const;
    void set_user_id(uint32_t id);

    QString first_name() const;
    void set_first_name(const QString& name);

    QString last_name() const;
    void set_last_name(const QString& name);

    QString user_name() const;
    void set_user_name(const QString& name);

    QString lang() const;
    void set_lang(const QString& name);

    qint64 private_chat_id() const;
    void set_private_chat_id(qint64 private_chat_id);
private:
    int32_t id_;
    uint32_t user_id_;
    qint64 private_chat_id_;
    QString first_name_, last_name_, user_name_, lang_;
};

} // namespace DB

using Tg_User = DB::Tg_User;

} // namespace Das

#endif // DAS_DATABASE_TG_USER_H
