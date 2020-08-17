#ifndef DAS_STATUS_HELPER_H
#define DAS_STATUS_HELPER_H

#include <Helpz/db_base.h>

#include <Das/db/dig_status_type.h>
#include <Das/db/dig_status.h>

#include "scheme_info.h"

namespace Das {

class Status_Helper
{
public:
    struct Section
    {
        bool operator == (uint32_t id) const { return id_ == id; }
        uint32_t id_;
        std::string name_;
        struct Group
        {
            bool operator == (uint32_t id) const { return id_ == id; }
            uint32_t id_;
            std::string name_;

            struct Status
            {
                uint32_t type_id_;
                std::string icon_, text_;
            };

            std::vector<Status> status_vect_;
        };
        std::vector<Group> group_vect_;
    };

    static std::vector<Section> get_group_names(const QSet<uint32_t>& group_id_set, Helpz::DB::Base& db, const Scheme_Info& scheme);
    static void fill_group_status_text(std::vector<Section>& group_names, const DIG_Status_Type& info, const DIG_Status& item, bool is_up = false);
    static void fill_dig_status_text(std::vector<Section>& group_names, const QVector<DIG_Status_Type>& info_vect, const DIG_Status& item, bool is_up = false);
};

} // namespace Das

#endif // DAS_STATUS_HELPER_H
