#include "sensormodel.h"

namespace Das {
namespace Gui {

SensorModel::SensorModel() { reset(); }

void SensorModel::reset()
{
//    disconnect(0,0, this, &SensorModel::changes);
    beginResetModel();

    items.clear();
    for (Section* sct: prj()->sections())
    {
//        connect(sct.get(), &Section::valueChanged,
//                this, &SensorModel::setValue);

        for (Device_item_Group* group: sct->groups())
            for (Device_Item* item: group->items())
            {
                connect(item, &Device_Item::value_changed, this, &SensorModel::changes);
                items.push_back( SensorItem(sct, item, group) );
            }
    }

    endResetModel();
}

void SensorModel::changes(uint32_t user_id)
{
    auto item = static_cast<Device_Item*>(sender());
    for (size_t i = 0; i < items.size(); i++)
        if (items.at(i).item == item)
            emit dataChanged(index(i), index(i));
}

QVariant SensorModel::data(const SensorItem &item, int role) const
{
    switch (role) {
    case TitleRole:
        return item.item->display_name();
    case ValueRole:
        return valueOfType( item.item );
    case SignRole:
        if (!item.item->is_control() && item.item->is_connected())
            return signByDevItem(prj(), item.item->type_id());
        return QVariant();
    default: return QVariant();
    }
}

QHash<int, QByteArray> SensorModel::roleNames() const
{
    return QMultiHash<int, QByteArray>(
    {
        {SignRole, "sign"}
    }) + TemplateModel::roleNames();
}

QVariant SensorModel::valueOfType(Device_Item* item) const
{
    if (!item->is_connected())
        return tr("Не подключен");

    auto val = item->value();

    if (item->register_type() == Device_Item_Type::RT_COILS ||
             item->register_type() == Device_Item_Type::RT_DISCRETE_INPUTS)
        return val.toBool() ? tr("Включено") : tr("Выключено");
    else
    {
        if (val.type() == QVariant::Double)
            return QString::number(val.toDouble());
        return val;
    }
}

} // namespace Gui
} // namespace Das
