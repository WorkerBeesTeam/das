#include <Helpz/zfile.h>

#include <QCryptographicHash>

#include "mnemoscheme.h"

namespace Das {
namespace DB {

/*static*/ std::string Mnemoscheme::_base_dir_path = "/opt/das/blob/mnemoscheme/";

Mnemoscheme::Mnemoscheme(uint id, const QString &title, const QByteArray &hash) :
    Base_Type{id}, _title{title}, _hash{hash} {}

const QString &Mnemoscheme::title() const { return _title; }
void Mnemoscheme::set_title(const QString &title) { _title = title; }

const QByteArray &Mnemoscheme::hash() const { return _hash; }
void Mnemoscheme::set_hash(const QByteArray &hash) { _hash = hash; }

QByteArray Mnemoscheme::get_data() const
{
    const std::string file_path = _base_dir_path + std::to_string(id()) + ".svg";
    const std::string raw_data = Helpz::File::read_all(file_path);
    QByteArray data = QByteArray::fromStdString(raw_data);
    if (QByteArray::fromHex(_hash) == QCryptographicHash::hash(data, QCryptographicHash::Sha1))
        return data;
    return {};
}

void Mnemoscheme::set_data(const QByteArray &data)
{
    const std::string file_path = _base_dir_path + std::to_string(id()) + ".svg";
    if (Helpz::File::write(file_path, data.constData(), data.size()))
        _hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
}

QDataStream& operator<<(QDataStream& ds, const Mnemoscheme& item)
{
    return ds << static_cast<const Base_Type&>(item) << item.title()
       << item.get_data();
}

QDataStream& operator>>(QDataStream& ds, Mnemoscheme& item)
{
    QByteArray data;
    ds >> static_cast<Base_Type&>(item) >> item._title >> data;
    item.set_data(data);
    return ds;
}

} // namespace DB
} // namespace Das
