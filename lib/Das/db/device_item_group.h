#ifndef DAS_DATABASE_DEVICE_ITEM_GROUP_H
#define DAS_DATABASE_DEVICE_ITEM_GROUP_H

#include <QDataStream>

#include <Helpz/db_meta.h>

#include <Das/daslib_global.h>
#include <Das/db/schemed_model.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Device_Item_Group : public Schemed_Model
{
    HELPZ_DB_META(Device_Item_Group, "device_item_group", "dig", DB_A(id), DB_ANS(title), DB_A(section_id),
                  DB_A(type_id), DB_AN(parent_id), DB_A(scheme_id))
public:
    Device_Item_Group(uint32_t id = 0, const QString& title = {}, uint32_t section_id = 0, uint32_t type_id = 0, uint32_t parent_id = 0);
    Device_Item_Group(Device_Item_Group&&) = default;
    Device_Item_Group(const Device_Item_Group&) = default;
    Device_Item_Group& operator=(Device_Item_Group&&) = default;
    Device_Item_Group& operator=(const Device_Item_Group&) = default;

    uint32_t id() const;
    void set_id(uint32_t id);

    QString title() const;
    void set_title(const QString& title);

    uint32_t section_id() const;
    void set_section_id(uint32_t section_id);

    uint32_t type_id() const;
    void set_type_id(uint32_t type_id);

    uint32_t parent_id() const;
    void set_parent_id(uint32_t parent_id);
private:
    uint32_t id_, section_id_, type_id_, parent_id_;
    QString title_;

    friend QDataStream &operator>>(QDataStream &ds, Device_Item_Group& group);
};

QDataStream &operator>>(QDataStream &ds, Device_Item_Group& group);
QDataStream &operator<<(QDataStream &ds, const Device_Item_Group& group);

} // namespace DB
} // namespace Das

#endif // DAS_DATABASE_DEVICE_ITEM_GROUP_H
