#ifndef DAS_DATABASE_TRANSLATION_H
#define DAS_DATABASE_TRANSLATION_H

#include <Helpz/db_meta.h>

#include "base_type.h"

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT Translation : public Base_Type
{
    HELPZ_DB_META(Translation, "translation", "tr", 4, DB_A(id), DB_A(lang), DB_A(data), DB_A(scheme_id))
public:
    Translation(uint32_t id = 0, const QString& lang = {}, const QString& data = {});

    QString lang() const;
    void set_lang(const QString& lang);

    QString data() const;
    void set_data(const QString &data);

private:
    QString data_;

    friend QDataStream& operator>>(QDataStream& ds, Translation& item);
};

QDataStream& operator<<(QDataStream& ds, const Translation& item);
QDataStream& operator>>(QDataStream& ds, Translation& item);

} // namespace Database

using Translation = Database::Translation;

} // namespace Das

#endif // DAS_DATABASE_TRANSLATION_H
