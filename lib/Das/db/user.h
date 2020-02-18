#ifndef DAS_DATABASE_USER_H
#define DAS_DATABASE_USER_H

#include <QDateTime>
#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT User
{
    HELPZ_DB_META(User, "user", "u", 13, DB_A(id), DB_A(is_superuser), DB_A(is_active), DB_A(is_staff),
                  DB_A(date_joined), DB_A(last_login), DB_A(username), DB_A(first_name), DB_A(last_name),
                  DB_A(email), DB_A(password), DB_A(need_to_change_password), DB_A(phone_number))
public:
    User(uint32_t id = 0, bool is_superuser = false, bool is_staff = false, bool is_active = true,
              const QDateTime& date_joined = {}, const QDateTime& last_login = {}, const QString& username = {},
              const QString& first_name = {}, const QString& last_name = {}, const QString& email = {},
              const QString& password = {}, bool need_to_change_password = false, const QString& phone_number = {});
    User(User&&) = default;
    User(const User&) = default;
    User& operator =(User&&) = default;
    User& operator =(const User&) = default;

    uint32_t id() const;
    void set_id(uint32_t id);

    uint8_t flags() const;
    void set_flags(uint8_t flags);

    bool is_superuser() const;
    void set_is_superuser(bool val);

    bool is_active() const;
    void set_is_active(bool val);

    bool is_staff() const;
    void set_is_staff(bool val);

    bool need_to_change_password() const;
    void set_need_to_change_password(bool val);

    QDateTime date_joined() const;
    void set_date_joined(const QDateTime &date_joined);

    QDateTime last_login() const;
    void set_last_login(const QDateTime &last_login);

    QString username() const;
    void set_username(const QString &username);

    QString first_name() const;
    void set_first_name(const QString &first_name);

    QString last_name() const;
    void set_last_name(const QString &last_name);

    QString email() const;
    void set_email(const QString &email);

    QString password() const;
    void set_password(const QString &password);

    QString phone_number() const;
    void set_phone_number(const QString &phone_number);

private:
    enum Flags {
        IS_SUPERUSER            = 1,
        IS_ACTIVE               = 2,
        IS_STAFF                = 4,
        NEED_TO_CHANGE_PASSWORD = 8,
    };

    uint32_t id_;
    uint8_t flags_;
    QDateTime date_joined_, last_login_;
    QString username_, first_name_, last_name_, email_, password_, phone_number_;

    friend QDataStream& operator>>(QDataStream& ds, User& item);
};

QDataStream& operator<<(QDataStream& ds, const User& item);
QDataStream& operator>>(QDataStream& ds, User& item);

// ----------------------------------------------------------------------------------------------------

class DAS_LIBRARY_SHARED_EXPORT User_Groups
{
    HELPZ_DB_META(User_Groups, "user_groups", "ug", 3, DB_A(id), DB_A(user_id), DB_A(group_id))
public:
    User_Groups(uint32_t id = 0, uint32_t user_id = 0, uint32_t group_id = 0);

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t user_id() const;
    void set_user_id(uint32_t user_id);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);

private:
    uint32_t id_, user_id_, group_id_;

    friend QDataStream& operator>>(QDataStream& ds, User_Groups& item);
};

QDataStream& operator<<(QDataStream& ds, const User_Groups& item);
QDataStream& operator>>(QDataStream& ds, User_Groups& item);

} // namespace DB

using User = DB::User;
using User_Groups = DB::User_Groups;

} // namespace Das

#endif // DAS_DATABASE_USER_H
