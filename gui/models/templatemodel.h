#ifndef DAS_GUI_TEMPLATEMODEL_H
#define DAS_GUI_TEMPLATEMODEL_H

#include <QAbstractListModel>

#include <Das/proto_scheme.h>

namespace Das {
namespace Gui {

struct TemplateGlobal {
    Proto_Scheme *prj();
    const Proto_Scheme *prj() const;
};

struct TemplateItem : public TemplateGlobal {
    TemplateItem(Section* section);
    Section* sct;
};

template<class T, class Item>
class TemplateModel : public QAbstractListModel, public TemplateGlobal {
public:
    virtual ~TemplateModel() = default;

    enum Roles {
        NameRole = Qt::UserRole + 1,
        TitleRole,
        ValueRole,
        UserRolesCount
    };

    virtual void reset() = 0;

    int rowCount(const QModelIndex &p = QModelIndex()) const override {
        return p.isValid() ? 0 : items.size();
    }

    virtual QHash<int, QByteArray> roleNames() const override
    {
        return QHash<int, QByteArray>(
        {
            {NameRole, "name"},
            {TitleRole, "title"},
            {ValueRole, "value"}
        });
    }

    QVariant data(const QModelIndex &idx, int role) const override
    {
        switch ((Roles)role) {
        case NameRole:
            return items.at(idx.row()).sct->name();
        default:
            return data( items.at(idx.row()), role );
        }
    }

    bool setData(const QModelIndex &idx, const QVariant &value, int role) override
    {
        int ok_role = role > Qt::UserRole ? role : -1;

        setData( items.at(idx.row()), value, role, ok_role );

        if (ok_role >= 0)
            emit dataChanged(idx, idx, {ok_role});
        return ok_role >= 0;
    }
protected:
    virtual QVariant data(const Item& item, int role) const = 0;
    virtual void setData(Item& item, const QVariant &value, int role, int& ok_role) {
        Q_UNUSED(item)
        Q_UNUSED(value)
        Q_UNUSED(role)
        Q_UNUSED(ok_role)
    }

    std::vector<Item> items;
};

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_TEMPLATEMODEL_H
