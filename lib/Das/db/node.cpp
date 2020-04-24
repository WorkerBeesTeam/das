#include "node.h"

namespace Das {
namespace DB {

Node::Node(uint32_t id, const QString& name, uint32_t parent_id, uint32_t type_id) :
    Base_Type(id, name),
    parent_id_(parent_id), type_id_(type_id)
{
}

uint32_t Node::parent_id() const { return parent_id_; }
void Node::set_parent_id(uint32_t parent_id) { parent_id_ = parent_id; }

uint32_t Node::type_id() const { return type_id_; }
void Node::set_type_id(uint32_t type_id) { type_id_ = type_id; }

QDataStream &operator<<(QDataStream &ds, const Node &item)
{
    return ds << static_cast<const Base_Type&>(item) << item.parent_id() << item.type_id();
}

QDataStream &operator>>(QDataStream &ds, Node &item)
{
    return ds >> static_cast<Base_Type&>(item) >> item.parent_id_ >> item.type_id_;
}

} // namespace DB
} // namespace Das
