#include <Helpz/db_builder.h>

#include "scheme_info.h"

namespace Das {

Scheme_Info::Scheme_Info(Scheme_Info *obj) :
    _id(obj->_id), _extending_id_set(obj->_extending_id_set), _scheme_groups(obj->_scheme_groups) {}

Scheme_Info::Scheme_Info(uint32_t id, const std::set<uint32_t> &ids, const std::set<uint32_t>& scheme_groups) :
    _id(id), _extending_id_set(ids), _scheme_groups(scheme_groups)
{
}

uint32_t Scheme_Info::id() const { return _id; }
void Scheme_Info::set_id(uint32_t id)
{
    _id = id;
    _sql_cache.clear();
}

//QString Scheme_Info::scheme_title() const { return scheme_title_; }
//void Scheme_Info::set_scheme_title(const QString& title) { scheme_title_ = title; }

const std::set<uint32_t> &Scheme_Info::extending_scheme_ids() const { return _extending_id_set; }
void Scheme_Info::set_extending_scheme_ids(const std::set<uint32_t> &ids)
{
    _extending_id_set = ids;
    _sql_cache.clear();
}
void Scheme_Info::set_extending_scheme_ids(std::set<uint32_t> &&ids)
{
    _extending_id_set = std::move(ids);
    _sql_cache.clear();
}

const QString &Scheme_Info::ids_to_sql() const
{
    if (_sql_cache.isEmpty())
        _sql_cache = Helpz::DB::get_db_field_in_sql("scheme_id", _extending_id_set, _id);
    return _sql_cache;
}

//uint32_t Scheme_Info::parent_id_or_id() const
//{
//    return _parent_id ? _parent_id : _id;
//}

const std::set<uint32_t> &Scheme_Info::scheme_groups() const
{
    return _scheme_groups;
}

void Scheme_Info::set_scheme_groups(const std::set<uint32_t> &scheme_groups)
{
    _scheme_groups = scheme_groups;
//    std::sort(scheme_groups_.begin(), scheme_groups_.end());
}

void Scheme_Info::set_scheme_groups(std::set<uint32_t>&& scheme_groups)
{
    _scheme_groups = std::move(scheme_groups);
//    std::sort(scheme_groups_.begin(), scheme_groups_.end());
}

bool Scheme_Info::check_scheme_groups(const std::set<uint32_t>& scheme_groups) const
{
    for (uint32_t scheme_group_id: _scheme_groups)
        if (scheme_groups.find(scheme_group_id) != scheme_groups.cend())
            return true;
    return false;
}

bool Scheme_Info::operator ==(const Scheme_Info& o) const
{
    return _id == o._id;
}

} // namespace Das
