#include "scheme_info.h"

namespace Das {

Scheme_Info::Scheme_Info(Scheme_Info *obj) : id_(obj->id_), scheme_groups_(obj->scheme_groups_) {}

Scheme_Info::Scheme_Info(uint32_t id, const std::set<uint32_t>& scheme_groups) :
    id_(id), scheme_groups_(scheme_groups)
{
}

uint32_t Scheme_Info::id() const { return id_; }
void Scheme_Info::set_id(uint32_t id) { id_ = id; }

//QString Scheme_Info::scheme_title() const { return scheme_title_; }
//void Scheme_Info::set_scheme_title(const QString& title) { scheme_title_ = title; }

uint32_t Scheme_Info::parent_id() const { return parent_id_; }
void Scheme_Info::set_parent_id(uint32_t id) { parent_id_ = id; }

const std::set<uint32_t> &Scheme_Info::scheme_groups() const
{
    return scheme_groups_;
}

void Scheme_Info::set_scheme_groups(const std::set<uint32_t> &scheme_groups)
{
    scheme_groups_ = scheme_groups;
//    std::sort(scheme_groups_.begin(), scheme_groups_.end());
}

void Scheme_Info::set_scheme_groups(std::set<uint32_t>&& scheme_groups)
{
    scheme_groups_ = std::move(scheme_groups);
//    std::sort(scheme_groups_.begin(), scheme_groups_.end());
}

bool Scheme_Info::check_scheme_groups(const std::set<uint32_t>& scheme_groups) const
{
    for (uint32_t scheme_group_id: scheme_groups_)
        if (scheme_groups.find(scheme_group_id) != scheme_groups.cend())
            return true;
    return false;
}

bool Scheme_Info::operator ==(const Scheme_Info& o) const
{
    return id_ == o.id_;
}

} // namespace Das
