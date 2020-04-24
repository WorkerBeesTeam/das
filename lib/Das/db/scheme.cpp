#include "scheme.h"

namespace Das {
namespace DB {

Scheme::Scheme(uint32_t id, const QString &name, const QString &title, const QString &address,
               const QString &description, uint32_t city_id, uint32_t company_id, const QDateTime &last_usage,
               const QString &version, const QString &using_key, uint32_t parent_id) :
    Titled_Type(id, name, title),
    city_id_(city_id), company_id_(company_id), parent_id_(parent_id),
    address_(address), description_(description), version_(version), using_key_(using_key),
    last_usage_(last_usage)
{
}

QString Scheme::address() const { return address_; }
void Scheme::set_address(const QString &address) { address_ = address; }

QString Scheme::description() const { return description_; }
void Scheme::set_description(const QString &description) { description_ = description; }

QString Scheme::version() const { return version_; }
void Scheme::set_version(const QString &version) { version_ = version; }

QString Scheme::using_key() const { return using_key_; }
void Scheme::set_using_key(const QString &using_key) { using_key_ = using_key; }

uint32_t Scheme::city_id() const { return city_id_; }
void Scheme::set_city_id(uint32_t city_id) { city_id_ = city_id; }

uint32_t Scheme::company_id() const { return company_id_; }
void Scheme::set_company_id(uint32_t company_id) { company_id_ = company_id; }

uint32_t Scheme::parent_id() const { return parent_id_; }
void Scheme::set_parent_id(uint32_t parent_id) { parent_id_ = parent_id; }

QDateTime Scheme::last_usage() const { return last_usage_; }
void Scheme::set_last_usage(const QDateTime &last_usage) { last_usage_ = last_usage; }

} // namespace DB
} // namespace Das
