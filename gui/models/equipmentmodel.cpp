#include <algorithm>

#include <QTimerEvent>

#include "equipmentmodel.h"
#include "serverapicall.h"

namespace Das {
namespace Gui {

EquipmentGroupModel::EquipmentGroupModel(Device_item_Group *group) :
    QAbstractListModel()
{
    connect(group, &Device_item_Group::control_changed, this, &EquipmentGroupModel::changed);

    std::vector<uint> types;

    for (Device_Item* item: group->items())
        if (item->is_control() &&
                std::find(types.cbegin(), types.cend(), item->type_id()) == types.cend() )
        {
            items.push_back( item );
            types.push_back( item->type_id() );
        }
}

QHash<int, QByteArray> EquipmentGroupModel::roleNames() const
{
    return QHash<int, QByteArray>(
    {
        {TitleRole, "title"},
        {ValueRole, "value"},
        {IsCoilsRole, "isCoils"},
    });
}

int EquipmentGroupModel::rowCount(const QModelIndex &p) const
{
    return p.isValid() ? 0 : items.size();
}

QVariant EquipmentGroupModel::data(const QModelIndex &idx, int role) const
{
    const Device_Item* item = items.at(idx.row());

    switch (role) {
    case TitleRole:     return item->display_name();
    case ValueRole:     return item->value();
    case IsCoilsRole:   return item->register_type() == Device_Item_Type::RT_COILS;
    default: return QVariant();
    }
}

bool EquipmentGroupModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    switch (role) {
    case ValueRole:
        if (idx.row() < items.size())
        {
            Device_Item* item = items.at(idx.row());
            if (item->value() != value)
            {
                ServerApiCall::instance()->websock()->sendDevItemValue(item->id(), value);
                emit dataChanged(idx, idx, {ValueRole});
                return true;
            }
        }
        return false;
    default:
        return false;
    }
}

void EquipmentGroupModel::changed(Device_Item *item)
{
    for (int i = 0; i < items.size(); i++)
    {
        Device_Item* m_item = items.at(i);
        if (m_item->type_id() == item->type_id())
        {
            QVector<int> roles{ValueRole};

            auto idx = index(i);
            emit dataChanged(idx, idx, roles);
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------------------

EquipmentModel::EquipmentModel() { reset(); }

void EquipmentModel::reset()
{
    beginResetModel();

    items.clear();

    for (Section* sct: prj()->sections())
        for (Device_item_Group* group: sct->groups())
        {
            EquipmentItem item(group);
            if (item.model->rowCount())
            {
                connect(group, &Device_item_Group::mode_changed, this, &EquipmentModel::modeChanged);
                items.push_back( item );
            }
        }

    endResetModel();
}

QHash<int, QByteArray> EquipmentModel::roleNames() const
{
    return QMultiHash<int, QByteArray>( {
        { ModeRole, "auto" },
        { ModelRole, "model" } }) + TemplateModel::roleNames();
}

QVariant EquipmentModel::data(const EquipmentItem &item, int role) const
{
    switch (role) {
    case TitleRole: return item.group->name();
    case ModeRole:  return item.group->mode_id() == 2;
    case ModelRole: return QVariant::fromValue(item.model);
    default:
        return QVariant();
    }
}

void EquipmentModel::setData(EquipmentItem &item, const QVariant &value, int role, int &ok_role)
{
    switch (role) {
    case ModeRole:
        if (item.group->mode_id() == (value.toBool() ? 2 : 1))
            ok_role = -1;
        else
        {
            item.group->set_mode( value.toBool() ? 2 : 1 );
            ServerApiCall::instance()->websock()->sendGroupMode(item.group->id(), value.toBool() ? 2 : 1);
        }
        break;
    }
}

void EquipmentModel::modeChanged(uint)
{
    auto group = static_cast<Device_item_Group*>(sender());
    for (uint i = 0; i < items.size(); i++)
        if (items.at(i).group == group)
        {
            QVector<int> roles{ModeRole};

            auto idx = index(i);
            emit dataChanged(idx, idx, roles);
            break;
        }
}

} // namespace Gui
} // namespace Das
