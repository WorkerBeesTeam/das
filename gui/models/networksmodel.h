#ifndef DAS_GUI_NETWORKSMODEL_H
#define DAS_GUI_NETWORKSMODEL_H

#include <QAbstractListModel>
#include <QNetworkInterface>

namespace Das {
namespace Gui {

template<class T>
class SimpleModel : public QAbstractListModel {
protected:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ValueRole,
    };

    QList<T> m_items;

    int rowCount(const QModelIndex &p = QModelIndex()) const override
    {
        return p.isValid() ? 0 : m_items.size();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return QHash<int, QByteArray>(
                    { {NameRole, "name"},
                      {ValueRole, "value"}, });
    }
};

class NetworksModel : public SimpleModel<QNetworkInterface>
{
public:
    NetworksModel();
    QVariant data(const QModelIndex &index, int role) const override;
};

class WiFiModel : public SimpleModel<QString>
{
public:
    WiFiModel();
    QVariant data(const QModelIndex &index, int role) const override;
};

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_NETWORKSMODEL_H
