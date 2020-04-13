#ifndef DAS_DB_SCHEME_H
#define DAS_DB_SCHEME_H

#include <QDateTime>

#include <Helpz/db_meta.h>

#include <Das/db/base_type.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Scheme : public Titled_Type
{
    HELPZ_DB_META(Scheme, "scheme", "s", 11, DB_A(id), DB_A(name), DB_A(title), DB_A(address), DB_A(description),
                  DB_A(city_id), DB_A(company_id), DB_A(last_usage), DB_A(version), DB_A(using_key), DB_AN(parent_id))
public:
    Scheme(uint32_t id = 0, const QString& name = QString(), const QString& title = QString(), const QString& address = QString(),
           const QString& description = QString(), uint32_t city_id = 0, uint32_t company_id = 0, const QDateTime& last_usage = QDateTime(),
           const QString& version = QString(), const QString& using_key = QString(), uint32_t parent_id = 0);

    QString address() const;
    void set_address(const QString& address);

    QString description() const;
    void set_description(const QString& description);

    uint32_t city_id() const;
    void set_city_id(uint32_t city_id);

    uint32_t company_id() const;
    void set_company_id(uint32_t company_id);

    QDateTime last_usage() const;
    void set_last_usage(const QDateTime& last_usage);

    QString version() const;
    void set_version(const QString& version);

    QString using_key() const;
    void set_using_key(const QString& using_key);

    uint32_t parent_id() const;
    void set_parent_id(uint32_t parent_id);
private:
    uint32_t city_id_, company_id_, parent_id_;
    QString address_, description_, version_, using_key_;
    QDateTime last_usage_;
};

} // namespace DB
} // namespace Das

#endif // DAS_DB_SCHEME_H
