#ifndef DAS_DB_HELP_H
#define DAS_DB_HELP_H

#include <Helpz/db_meta.h>

#include "base_type.h"

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Help : public Base_Type
{
    HELPZ_DB_META(Help, "help", "h", DB_A(id), DB_AN(parent_id), DB_A(name), DB_A(text), DB_AN(scheme_id))
public:
    Help(uint32_t id = 0, uint32_t parent_id = 0, const QString& name = QString(),
         const QString& text = QString(), uint32_t scheme_id = 0);

    uint32_t parent_id() const;
    void set_parent_id(uint32_t id);

    const QString& text() const;
    void set_text(const QString& text);
private:
    uint32_t _parent_id;
    QString _text;
};

} // namespace DB
} // namespace Das

#endif // DAS_DB_HELP_H
