#include "authentication_info.h"

namespace Das {

Authentication_Info::Authentication_Info(const QString& login, const QString& password, const QString& scheme_name,
                                         const QUuid& using_key) :
    login_(login), password_(password), scheme_name_(scheme_name), using_key_(using_key)
{
}

QString Authentication_Info::login() const { return login_; }
void Authentication_Info::set_login(const QString &login) { login_ = login; }

QString Authentication_Info::password() const { return password_; }
void Authentication_Info::set_password(const QString &password) { password_ = password; }

QString Authentication_Info::scheme_name() const { return scheme_name_; }
void Authentication_Info::set_scheme_name(const QString& scheme_name) { scheme_name_ = scheme_name; }

QUuid Authentication_Info::using_key() const { return using_key_; }
void Authentication_Info::set_using_key(const QUuid &using_key) { using_key_ = using_key; }

Authentication_Info::operator bool() const
{
    return !login_.isEmpty() && !password_.isEmpty() && !scheme_name_.isEmpty();
}

QDataStream &operator <<(QDataStream &ds, const Authentication_Info &info)
{
    return ds << info.login() << info.password() << info.scheme_name() << info.using_key();
}

QDataStream &operator >>(QDataStream &ds, Authentication_Info &info)
{
    return ds >> info.login_ >> info.password_ >> info.scheme_name_ >> info.using_key_;
}

} // namespace Das
