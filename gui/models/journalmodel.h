#ifndef JOURNALMODEL_H
#define JOURNALMODEL_H

#include <QAbstractListModel>

#include <Das/device_item.h>
#include "journaldata.h"

#include <deque>


namespace Das {
namespace Gui {


class JournalModel: public QAbstractListModel {
    Q_OBJECT
public:
    /// Constructor
    JournalModel(QObject* parent = nullptr);
    /// Destructor
    ~JournalModel() override = default;

    /// returns data for index and role
    QVariant data(const QModelIndex &idx, int role) const override;

    /// enum data roles
    enum Roles {
        DateMsecRole = Qt::UserRole + 1,    // Date
        CategoryeRole,                      // Category
        MessageRole,                        // Message
        NameRole,                           // User name
        EventTypeRole                       // Event type
    };

    /// returns parent which is always null
    QModelIndex parent(const QModelIndex &) const override { return {}; }

    /// returns row count
    int rowCount(const QModelIndex & parent) const override;

    /// returns rolenames hash
    QHash<int, QByteArray> roleNames() const override;

    /// informs model that rows are about to be inserted
    void beforeInsertRows(int first, int last);

    /// informs model that rows are inserted
    /// must be used in the same portion of code with beforeInsertRows function
    void afterInsertRows();

    /// informs model, that rows are about to be removed
    void beforeRemoveRows(int first, int last);

    /// informs model, that rows were removed
    void afterRemoveRows();

public slots:
    /// test method for debugging model
    quint64 items_count() const;

signals:
    void positionChanged(QVariant position, QVariant top, QVariant bottom);
    void requestOldItems();
    void requestNewItems();

private:
    JournalModelData data_;
};

} }

#endif // JOURNALMODEL_H
