#ifndef DAS_DB_NODE_H
#define DAS_DB_NODE_H

#include <Helpz/db_meta.h>

#include "base_type.h"

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Node : public Base_Type
{
    HELPZ_DB_META(Node, "node", "n", DB_A(id), DB_A(name), DB_A(parent_id), DB_A(type_id), DB_A(scheme_id))
public:
    Node(uint32_t id = 0, const QString& name = {}, uint32_t parent_id = 0, uint32_t type_id = 0);

    uint32_t parent_id() const;
    void set_parent_id(uint32_t parent_id);

    uint32_t type_id() const;
    void set_type_id(uint32_t type_id);
private:
    uint32_t parent_id_, type_id_;

    friend QDataStream &operator>>(QDataStream& ds, Node& node);
};

QDataStream &operator>>(QDataStream& ds, Node& node);
QDataStream &operator<<(QDataStream& ds, const Node& node);

} // namespace DB
} // namespace Das

#endif // DAS_DB_NODE_H
