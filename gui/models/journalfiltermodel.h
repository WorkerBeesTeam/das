#ifndef JOURNALFILTERMODEL_H
#define JOURNALFILTERMODEL_H

#include <QSortFilterProxyModel>
#include "journalmodel.h"

namespace Das {
namespace Gui {

class JournalFilterModel : public QSortFilterProxyModel
{
public:
    JournalFilterModel();

    void setCriterion(const QString& criterion);

    void setSourceModel(QAbstractItemModel *sourceModel) override;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    QString criterion_;
    JournalModel * model_;
};

}
}

#endif // JOURNALFILTERMODEL_H
