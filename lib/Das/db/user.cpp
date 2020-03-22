#include "user.h"

namespace Das {
namespace DB {

User::User(uint32_t id, bool is_superuser, bool is_staff, bool is_active,
            const QDateTime &date_joined, const QDateTime &last_login, const QString &username,
            const QString &first_name, const QString &last_name, const QString &email,
            const QString& password, bool need_to_change_password, const QString &phone_number) :
    id_(id), flags_(0), date_joined_(date_joined), last_login_(last_login),
    username_(username), first_name_(first_name), last_name_(last_name),
    email_(email), password_(password), phone_number_(phone_number)
{
    set_is_superuser(is_superuser);
    set_is_staff(is_staff);
    set_is_active(is_active);
    set_need_to_change_password(need_to_change_password);
}

uint32_t User::id() const { return id_; }
void User::set_id(uint32_t id) { id_ = id; }

uint8_t User::flags() const { return flags_; }
void User::set_flags(uint8_t flags) { flags_ = flags; }

bool User::is_superuser() const
{
    return flags_ & IS_SUPERUSER;
}

void User::set_is_superuser(bool val)
{
    flags_ ^= static_cast<uint8_t>((-(unsigned long)val ^ flags_) & IS_SUPERUSER);
}

bool User::is_active() const
{
    return flags_ & IS_ACTIVE;
}

void User::set_is_active(bool val)
{
    flags_ ^= static_cast<uint8_t>((-(unsigned long)val ^ flags_) & IS_ACTIVE);
}

bool User::is_staff() const
{
    return flags_ & IS_STAFF;
}

void User::set_is_staff(bool val)
{
    flags_ ^= static_cast<uint8_t>((-(unsigned long)val ^ flags_) & IS_STAFF);
}

bool User::need_to_change_password() const
{
    return flags_ & NEED_TO_CHANGE_PASSWORD;
}

void User::set_need_to_change_password(bool val)
{
    flags_ ^= static_cast<uint8_t>((-(unsigned long)val ^ flags_) & NEED_TO_CHANGE_PASSWORD);
}

QDateTime User::date_joined() const { return date_joined_; }
void User::set_date_joined(const QDateTime &date_joined) { date_joined_ = date_joined; }

QDateTime User::last_login() const { return last_login_; }
void User::set_last_login(const QDateTime &last_login) { last_login_ = last_login; }

QString User::username() const { return username_; }
void User::set_username(const QString &username) { username_ = username; }

QString User::first_name() const { return first_name_; }
void User::set_first_name(const QString &first_name) { first_name_ = first_name; }

QString User::last_name() const { return last_name_; }
void User::set_last_name(const QString &last_name) { last_name_ = last_name; }

QString User::email() const { return email_; }
void User::set_email(const QString &email) { email_ = email; }

QString User::password() const { return password_; }
void User::set_password(const QString& password) { password_ = password; }

QString User::phone_number() const { return phone_number_; }
void User::set_phone_number(const QString& phone_number) { phone_number_ = phone_number; }

QDataStream &operator<<(QDataStream &ds, const User &item)
{
    return ds << item.id() << item.flags() << item.date_joined() << item.last_login()
              << item.username() << item.first_name() << item.last_name() << item.email()
              << item.password() << item.phone_number();
}

QDataStream &operator>>(QDataStream &ds, User &item)
{
    return ds >> item.id_ >> item.flags_ >> item.date_joined_ >> item.last_login_
              >> item.username_ >> item.first_name_ >> item.last_name_ >> item.email_
              >> item.password_ >> item.phone_number_;
}

// ----------------------------------------------------------------------------------------------------

User_Groups::User_Groups(uint32_t id, uint32_t user_id, uint32_t group_id) :
    id_(id), user_id_(user_id), group_id_(group_id)
{
}

uint32_t User_Groups::id() const { return id_; }
void User_Groups::set_id(uint32_t id) { id_ = id; }

uint32_t User_Groups::user_id() const { return user_id_; }
void User_Groups::set_user_id(uint32_t user_id) { user_id_ = user_id; }

uint32_t User_Groups::group_id() const { return group_id_; }
void User_Groups::set_group_id(uint32_t group_id) { group_id_ = group_id; }

QDataStream &operator<<(QDataStream &ds, const User_Groups &item)
{
    return ds << item.id() << item.user_id() << item.group_id();
}

QDataStream &operator>>(QDataStream &ds, User_Groups &item)
{
    return ds >> item.id_ >> item.user_id_ >> item.group_id_;
}

} // namespace DB
} // namespace Das
