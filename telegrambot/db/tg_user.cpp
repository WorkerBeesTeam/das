#include "tg_user.h"

namespace Das {
namespace Database {

Tg_User::Tg_User(int32_t id, uint32_t user_id, const QString &first_name,
                 const QString &last_name, const QString &user_name, const QString &lang) :
    id_(id), user_id_(user_id), first_name_(first_name),
    last_name_(last_name), user_name_(user_name), lang_(lang)
{
}

int32_t Tg_User::id() const { return id_; }
void Tg_User::set_id(int32_t id) { id_ = id; }

uint32_t Tg_User::user_id() const { return user_id_; }
void Tg_User::set_user_id(uint32_t id) { user_id_ = id; }

QString Tg_User::first_name() const { return first_name_; }
void Tg_User::set_first_name(const QString &name) { first_name_ = name; }

QString Tg_User::last_name() const { return last_name_; }
void Tg_User::set_last_name(const QString &name) { last_name_ = name; }

QString Tg_User::user_name() const { return user_name_; }
void Tg_User::set_user_name(const QString &name) { user_name_ = name; }

QString Tg_User::lang() const { return lang_; }
void Tg_User::set_lang(const QString &name) { lang_ = name; }

} // namespace Database
} // namespace Das
