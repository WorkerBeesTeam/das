#include "node.h"

namespace Das {
namespace DB {

Node::Node(uint32_t id, const QString& name, uint32_t parent_id, uint32_t type_id, uint8_t order) :
    Named_Type{id, name},
    _order{order},
    _parent_id{parent_id}, _type_id{type_id}
{
}

uint32_t Node::parent_id() const { return _parent_id; }
void Node::set_parent_id(uint32_t parent_id) { _parent_id = parent_id; }

uint32_t Node::type_id() const { return _type_id; }
void Node::set_type_id(uint32_t type_id) { _type_id = type_id; }

uint8_t Node::order() const { return _order; }
void Node::set_order(uint8_t n) { _order = n; }

QDataStream &operator<<(QDataStream &ds, const Node &item)
{
    return ds << static_cast<const Named_Type&>(item) << item.parent_id() << item.type_id() << item.order();
}

QDataStream &operator>>(QDataStream &ds, Node &item)
{
    return ds >> static_cast<Named_Type&>(item) >> item._parent_id >> item._type_id >> item._order;
}

} // namespace DB
} // namespace Das
