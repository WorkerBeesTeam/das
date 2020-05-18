#ifndef DAS_REST_SCHEME_COPIER_H
#define DAS_REST_SCHEME_COPIER_H

#include <stdint.h>
#include <vector>
#include <map>

#include <Helpz/db_base.h>

namespace Das {

class Scheme_Copier
{
public:
    Scheme_Copier(uint32_t orig_id, uint32_t dest_id, bool is_dry_run);

    struct Item
    {
        enum Counter_Type { SCI_INSERTED, SCI_INSERT_ERROR, SCI_DELETED, SCI_DELETE_ERROR, SCI_UPDATED, SCI_UPDATE_ERROR, SCI_COUNT };
        std::size_t counter_[SCI_COUNT];
    };

    std::map<std::string, Item> result_;
private:
    void copy(uint32_t orig_id, uint32_t dest_id);

    template<typename T>
    void copy_table(Helpz::DB::Base& db, const QString& suffix, uint32_t orig_id, uint32_t dest_id,
                    std::map<uint32_t, uint32_t>* id_map = nullptr, const std::map<int, std::map<uint32_t, uint32_t>>& compare_id_map = {}, int self_column_index = -1);

    template<typename T>
    void fill_relations_ids(std::vector<T> &orig, const std::map<int, std::map<uint32_t, uint32_t>>& relations_id_map);

    template<typename T>
    void fill_items(std::vector<T> &orig, uint32_t orig_id, std::vector<T> &dest, uint32_t dest_id);

    template<typename T>
    void fill_vects(uint32_t dest_id, std::vector<T>& skipped_vect, std::vector<T>& insert_vect, std::vector<T>& delete_vect, std::vector<T>& update_vect,
                    std::map<uint32_t, uint32_t>* id_map, int self_column_index = -1);

    template<typename T>
    void sort_insert_vect(std::vector<T>& vect, int self_column_index = -1);

    template<typename T>
    void proc_vects(Helpz::DB::Base& db, const std::vector<T>& insert_vect, const std::vector<T>& delete_vect, const std::vector<T>& update_vect, std::map<uint32_t, uint32_t>* id_map);

    bool is_dry_run_;
};

} // namespace Das

#endif // DAS_REST_SCHEME_COPIER_H
