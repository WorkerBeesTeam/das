#ifndef DAS_AUTHENTICATION_INFO_H
#define DAS_AUTHENTICATION_INFO_H

#include <QUuid>

namespace Das {

class Authentication_Info
{
public:
    Authentication_Info(const QString& login, const QString& password, const QString& scheme_name, const QUuid& using_key);
    Authentication_Info() = default;
    Authentication_Info(const Authentication_Info&) = default;
    Authentication_Info(Authentication_Info&&) = default;
    Authentication_Info& operator=(Authentication_Info&&) = default;
    Authentication_Info& operator=(const Authentication_Info&) = default;

    QString login() const;
    void set_login(const QString &login);

    QString password() const;
    void set_password(const QString &password);

    QString scheme_name() const;
    void set_scheme_name(const QString& scheme_name);

    QUuid using_key() const;
    void set_using_key(const QUuid &using_key);

    operator bool() const;

private:
    QString login_, password_, scheme_name_;
    QUuid using_key_;

    friend QDataStream& operator >> (QDataStream& ds, Authentication_Info& info);
};

QDataStream& operator << (QDataStream& ds, const Authentication_Info& info);
QDataStream& operator >> (QDataStream& ds, Authentication_Info& info);

} // namespace Das

#endif // DAS_AUTHENTICATION_INFO_H
