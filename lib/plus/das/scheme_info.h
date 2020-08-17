#ifndef DAS_SCHEME_INFO_H
#define DAS_SCHEME_INFO_H

#include <string>
#include <set>

//#include <boost/asio.hpp>

#include <QTimeZone>

namespace Das {

//namespace database {
//class global;
//class base;
//}

enum Connection_State
{
    CS_SERVER_DOWN,
    CS_DISCONNECTED,
    CS_DISCONNECTED_JUST_NOW,
    CS_CONNECTED_JUST_NOW,
    CS_CONNECTED_SYNC_TIMEOUT,
    CS_CONNECTED,

    CS_CONNECTED_WITH_LOSSES = 0x40,
    CS_CONNECTED_MODIFIED = 0x80,
    CS_FLAGS = CS_CONNECTED_WITH_LOSSES | CS_CONNECTED_MODIFIED
};

class Scheme_Info
{
public:
    Scheme_Info(Scheme_Info* obj);
    Scheme_Info(uint32_t id = 0, const std::set<uint32_t>& ids = {}, const std::set<uint32_t>& scheme_groups = {});
    Scheme_Info(Scheme_Info&&) = default;
    Scheme_Info(const Scheme_Info&) = default;
    Scheme_Info& operator=(Scheme_Info&&) = default;
    Scheme_Info& operator=(const Scheme_Info&) = default;
    virtual ~Scheme_Info() = default;

    uint32_t id() const;
    void set_id(uint32_t id);

//    QString scheme_title() const;
//    void set_scheme_title(const QString& title);

    const std::set<uint32_t>& extending_scheme_ids() const;
    void set_extending_scheme_ids(const std::set<uint32_t>& ids);
    void set_extending_scheme_ids(std::set<uint32_t>&& ids);

    const QString& ids_to_sql() const;
//    uint32_t parent_id_or_id() const;

    const std::set<uint32_t>& scheme_groups() const;
    void set_scheme_groups(const std::set<uint32_t>& scheme_groups);
    void set_scheme_groups(std::set<uint32_t>&& scheme_groups);

    bool check_scheme_groups(const std::set<uint32_t>& scheme_groups) const;

    bool operator == (const Scheme_Info& o) const;
private:
    uint32_t _id;
    std::set<uint32_t> _extending_id_set, _scheme_groups;
    mutable QString _sql_cache;
};

} // namespace Das

Q_DECLARE_METATYPE(std::set<uint32_t>)
Q_DECLARE_METATYPE(Das::Scheme_Info)

#endif // DAS_SCHEME_INFO_H
