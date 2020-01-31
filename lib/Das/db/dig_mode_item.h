#ifndef DAS_DATABASE_DIG_MODE_ITEM_H
#define DAS_DATABASE_DIG_MODE_ITEM_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT DIG_Mode_Item : public Schemed_Model
{
    HELPZ_DB_META(DIG_Mode_Item, "dig_mode_item", "gmi", 4, DB_A(id), DB_A(group_id), DB_A(mode_id), DB_A(scheme_id))
public:
    DIG_Mode_Item(uint32_t group_id = 0, uint32_t mode_id = 0);

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t group_id() const;
    void set_group_id(uint32_t group_id);

    uint32_t mode_id() const;
    void set_mode_id(uint32_t mode_id);

private:
    uint32_t id_, group_id_, mode_id_;

    friend QDataStream &operator>>(QDataStream &ds, DIG_Mode_Item& item);
};

QDataStream &operator>>(QDataStream &ds, DIG_Mode_Item& item);
QDataStream &operator<<(QDataStream &ds, const DIG_Mode_Item& item);

} // namespace Database

using DIG_Mode_Item = Database::DIG_Mode_Item;

} // namespace Das

#endif // DAS_DATABASE_DIG_MODE_ITEM_H
