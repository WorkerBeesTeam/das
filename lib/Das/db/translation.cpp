#include "translation.h"

namespace Das {
namespace DB {

Translation::Translation(uint32_t id, const QString &lang, const QString &data) :
    Base_Type(id, lang), data_(data)
{
}

QString Translation::lang() const
{
    return name();
}

void Translation::set_lang(const QString &lang)
{
    set_name(lang);
}

QString Translation::data() const
{
    return data_;
}

void Translation::set_data(const QString &data)
{
    data_ = data;
}

QDataStream &operator<<(QDataStream &ds, const Translation &item)
{
    return ds << static_cast<const Base_Type&>(item) << item.data();
}

QDataStream &operator>>(QDataStream &ds, Translation &item)
{
    return ds >> static_cast<Base_Type&>(item) >> item.data_;
}

} // namespace DB
} // namespace Das
