#include "help.h"

namespace Das {
namespace DB {

Help::Help(uint32_t id, uint32_t parent_id, const QString &name, const QString &text, uint32_t scheme_id) :
    Named_Type(id, name, scheme_id),
    _parent_id(parent_id), _text(text)
{
}

uint32_t Help::parent_id() const { return _parent_id; }
void Help::set_parent_id(uint32_t id) { _parent_id = id; }

const QString &Help::text() const { return _text; }
void Help::set_text(const QString &text) { _text = text; }

} // namespace DB
} // namespace Das
