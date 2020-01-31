#include <cassert>

#include "journalfiltermodel.h"

namespace Das {
namespace Gui {

JournalFilterModel::JournalFilterModel()
{
}

void JournalFilterModel::setCriterion(const QString &criterion) {
    criterion_ = criterion;
}

void JournalFilterModel::setSourceModel(QAbstractItemModel *sourceModel) {
    auto * model = dynamic_cast<JournalModel*>(sourceModel);
    assert(model != nullptr && "JournalModel is required");
    model_ = model;
    QSortFilterProxyModel::setSourceModel(sourceModel);
}

bool JournalFilterModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
    auto index = model_->index(source_row, 0);
    QString category = model_->data(index, JournalModel::CategoryeRole).toString();
    QString message = model_->data(index, JournalModel::MessageRole).toString();

    return category.contains(criterion_, Qt::CaseInsensitive) || message.contains(criterion_, Qt::CaseInsensitive);
}

}
}
