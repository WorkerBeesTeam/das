#ifndef DAS_STATUS_HELPER_H
#define DAS_STATUS_HELPER_H

#include <Helpz/db_base.h>

#include <Das/db/dig_status_type.h>
#include <Das/db/dig_status.h>

namespace Das {

class Status_Helper
{
public:
    struct Section {
        bool operator == (uint32_t id) const { return id_ == id; }
        uint32_t id_;
        QString name_;
        struct Group {
            bool operator == (uint32_t id) const { return id_ == id; }
            uint32_t id_;
            QString name_;
            std::vector<std::pair<QString,QString>> status_text_vect_;
        };
        std::vector<Group> group_vect_;
    };

    static std::vector<Section> get_group_names(const QSet<uint32_t>& group_id_set, Helpz::Database::Base& db, uint32_t scheme_id);
    static void fill_group_status_text(std::vector<Section>& group_names, const DIG_Status_Type& info, const DIG_Status& item, bool is_up = false);
    static void fill_dig_status_text(std::vector<Section>& group_names, const QVector<DIG_Status_Type>& info_vect, const DIG_Status& item, bool is_up = false);
};

} // namespace Das

#endif // DAS_STATUS_HELPER_H
