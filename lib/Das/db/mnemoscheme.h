#ifndef DAS_DB_MNEMOSCHEME_H
#define DAS_DB_MNEMOSCHEME_H

#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace DB {

struct DAS_LIBRARY_SHARED_EXPORT Mnemoscheme : public Base_Type
{
    HELPZ_DB_META(Mnemoscheme, "mnemoscheme", "mns", DB_A(id), DB_A(title), DB_A(hash), DB_A(scheme_id))
public:
    Mnemoscheme(uint id = 0, const QString& title = {}, const QByteArray& hash = {});

    Mnemoscheme(const Mnemoscheme& other) = default;
    Mnemoscheme(Mnemoscheme&& other) = default;
    Mnemoscheme& operator=(const Mnemoscheme& other) = default;
    Mnemoscheme& operator=(Mnemoscheme&& other) = default;

    const QString& title() const;
    void set_title(const QString& title);

    const QByteArray& hash() const;
    void set_hash(const QByteArray& hash);

    QByteArray get_data() const;
    void set_data(const QByteArray& data);

    QString _title;
    QByteArray _hash;

    static std::string _base_dir_path;
};
QDataStream& operator<<(QDataStream& ds, const Mnemoscheme& item);
QDataStream& operator>>(QDataStream& ds, Mnemoscheme& item);

} // namespace DB
} // namespace Das

#endif // DAS_DB_MNEMOSCHEME_H
