#include "journalmodel.h"

#include <cmath>

namespace Das {
namespace Gui {

namespace
{

QString eventTypeToString(uint32_t event_type) {
    switch (event_type) {
    case 0: return QString::fromStdString("debug");
    case 1: return QString::fromStdString("warning");
    case 2: return QString::fromStdString("critical");
    case 3: return QString::fromStdString("fatal");
    case 4: return QString::fromStdString("info");
    case 5: return QString::fromStdString("user");
    default:
        break;
    }

    return {};
}

}


JournalModel::JournalModel(QObject* parent):
    QAbstractListModel{parent},
    data_{this}
{
    connect(this, &JournalModel::positionChanged, [&](QVariant position, QVariant top, QVariant bottom) {
        data_.onPositionChanged(
                    static_cast<int>(std::lround(position.toFloat() * 100)),
                    top.toInt(),
                    bottom.toInt());
    });

    connect(this, &JournalModel::requestOldItems, [&]() {
        data_.requestOldItems();
    });

    connect(this, &JournalModel::requestNewItems, [&]() {
        data_.requestNewItems();
    });
}


QVariant JournalModel::data(const QModelIndex &idx, int role) const
{
    std::size_t row = static_cast<std::size_t>(idx.row());
    const auto & item = data_[row];

    switch (role) {
    case DateMsecRole:
        return item.timestamp_msecs();
    case CategoryeRole:
        return item.category();
    case MessageRole:
        return item.text();
    case NameRole:
        return QString{};
    case EventTypeRole:
        return eventTypeToString(item.type_id());
    default:
        break;
    }

    return {};
}


int JournalModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(data_.size());
}


QHash<int, QByteArray> JournalModel::roleNames() const
{
    return {
        {DateMsecRole, "date_msec"},
        {CategoryeRole, "category"},
        {MessageRole, "message"},
        {NameRole, "user_name"},
        {EventTypeRole, "event_type"}
    };
}


void JournalModel::beforeInsertRows(int first, int last)
{
    beginInsertRows({}, first, last);
}


void JournalModel::afterInsertRows()
{
    endInsertRows();
}


void JournalModel::beforeRemoveRows(int first, int last)
{
    beginRemoveRows({}, first, last);
}


void JournalModel::afterRemoveRows()
{
    endRemoveRows();
}


quint64 JournalModel::items_count() const {
    return data_.size();
}

} }
