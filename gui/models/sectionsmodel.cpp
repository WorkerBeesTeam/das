#include <QColor>

#include "sectionsmodel.h"

namespace Das {
namespace Gui {

GroupStatusInfo::GroupStatusInfo(bool inform, uint32_t status, QString color, QStringList descriptions) :
    QObject(), m_inform(inform), m_status(status), m_color(color), m_descriptions(descriptions) {}

GroupStatusInfo::GroupStatusInfo(GroupStatusInfo &&o) :
    QObject(), m_inform(std::move(o.m_inform)), m_status(std::move(o.m_status)), m_color(std::move(o.m_color)), m_descriptions(std::move(o.m_descriptions)) {}

bool GroupStatusInfo::inform() const { return m_inform; }
uint32_t GroupStatusInfo::status() const { return m_status; }
const QString &GroupStatusInfo::color() const { return m_color; }
const QStringList &GroupStatusInfo::descriptions() const { return m_descriptions; }

SectionModelItem::SectionModelItem(Section *section, Device_item_Group* groupPtr) :
    TemplateItem(section), group(groupPtr)
{
    set_status();
}

QVector<int> SectionModelItem::set_status()
{
    QVector<int> roles;

    GroupStatusInfo info = getStatusInfo();
    if (status_color != info.color())
    {
        status_color = info.color();
        roles << SectionsModel::StatusColorRole << SectionsModel::ValueColorRole;
    }
    if (status_text != info.descriptions())
    {
        status_text = info.descriptions();
        roles << SectionsModel::StatusTextRole;
    }
    return roles;
}

GroupStatusInfo SectionModelItem::getStatusInfo() const
{
    /*
    if (!group || !group->section() || !group->section()->type_managers())
    {
        qCritical() << "EMPTY GROUP STATUS INFO";
        return {};
    }

    bool inform = false;
    QString color;
    QStringList descriptions;

    QMap<uint32_t, QStringList> statuses = group->statuses();
    uint32_t m_status = statuses.size() ? statuses.begin().key() : 0;

    if (m_status < Device_item_Group::valUser)
    {
        //        uint Device_item_type = typeMng->GroupTypeMng.displayType(group->type());
        //        bool isCtrl = Device_Item::isControl(typeMng->ItemTypeMng.type(Device_item_type).registerType);
        bool isCtrl = false;

        uint type_id;
        if (m_status & Device_item_Group::valNotAllOk && !isCtrl)
        {
            descriptions.push_back("Не все датчики в норме");
            type_id = Device_item_Group::valNotAllOk;
        }
        else if (m_status & Device_item_Group::valOk || (isCtrl && m_status & Device_item_Group::cvalOff))
            type_id = Device_item_Group::valOk;
        else
        {
            type_id = Device_item_Group::valUnknown;
            descriptions.push_back("Датчики не подключены");
        }

        color = group->section()->type_managers()->dig_status_category_mng_.type(type_id + 1).color;
    }
    else
    {
        uint32_t multiVal = Device_item_Group::valUser;
        auto checkStatus = [&](const DIG_Status_Type& type) -> bool {
            if (type.isMultiValue)
            {
                bool result = m_status & multiVal;
                multiVal <<= 1;
                return result;
            }
            else
                return m_status == type.value;
        };

        uint statusType = 0;
        const QVector<DIG_Status_Type>& types = group->section()->type_managers()->status_mng_.types();
        for (auto it = types.cbegin(); it != types.cend(); ++it)
            if (it->groupType_id == group->type_id() &&
                    checkStatus(*it) &&
                    it->type_id >= statusType)
            {
                if (!inform && it->inform)
                    inform = true;

                if (it->type_id > statusType)
                {
                    descriptions.clear();
                    statusType = it->type_id;
                }

                descriptions.push_back(it->title());
            }

        QString color_name = group->section()->type_managers()->dig_status_category_mng_.type(statusType).color;
        if (color_name.isEmpty())
            color = "#000000";
        else
            color = color_name;
    }

    return GroupStatusInfo{ inform, m_status, color, descriptions };
    */
    return {};
}

SectionsModel::SectionsModel()
{
    reset();
}

void SectionsModel::reset()
{
    beginResetModel();

    for (const SectionPtr& sct: prj()->sections())
        for (Device_item_Group* group: sct->groups())
        {
            connect(group, &Device_item_Group::item_changed, this, &SectionsModel::newValue);
            connect(group, &Device_item_Group::status_added, this, &SectionsModel::groupStatusChanged);
            connect(group, &Device_item_Group::status_removed, this, &SectionsModel::groupStatusChanged);
            items.push_back( SectionModelItem( sct, group) );
        }

    endResetModel();
}

QVariant SectionsModel::data(const SectionModelItem& item, int role) const
{
    switch (role) {
    case TitleRole:         return prj()->group_type_mng_.title(item.group->type_id());
    case NameRole:          return item.sct->name();
    case ValueRole:         return QString::number(item.value);
    case SignRole:          return signByDevItem(prj(), item.devDevice_item_type); // signByGroupType(g()->prj(), item.group->type());
    case StatusTextRole:    return item.status_text.join(" | ");
    case StatusColorRole:   return item.status_color;
    case ValueColorRole:
        return ((item.status_color.red() + item.status_color.green() + item.status_color.blue()) / 3) > 128 ?
                    QColor(Qt::black) : QColor(Qt::white);
    case TypeRole:          return item.group->type_id();
    case ParamRole:
    {
        return QVariant::fromValue(item.group->params());

        QObject* param = nullptr;

        qDebug() << "Section Mng Get Param" << item.group->type_id();

        //        param->deleteLater();
        QQmlEngine::setObjectOwnership(param, QQmlEngine::JavaScriptOwnership);
        return QVariant::fromValue(param);
    }
    default:
        return QVariant();
    }
}

void SectionsModel::setData(SectionModelItem& item, const QVariant &value, int role, int& ok_role)
{
    Q_UNUSED(item)
    Q_UNUSED(value)

    switch (role) {
    case ParamRole:
        ok_role = -1;
        break;
    case TitleRole:
    case NameRole:
    case ValueRole:
    case SignRole:
    case StatusColorRole:
    case TypeRole:
    default:
        ok_role = -1;
        break;
    }
}

QHash<int, QByteArray> SectionsModel::roleNames() const
{
    return QMultiHash<int, QByteArray>(
    {
                    {SignRole, "sign"},
                    {ValueColorRole, "textColor"},
                    {StatusColorRole, "statusColor"},
                    {StatusTextRole, "statusText"},
                    {TypeRole, "type"},
                    {ParamRole, "param"},
                }) + TemplateModel::roleNames();
}

void SectionsModel::groupStatusChanged(uint32_t /*status*/)
{
    auto group = static_cast<Device_item_Group*>(sender());
    for (uint i = 0; i < items.size(); i++)
        if (items.at(i).group == group)
        {
            auto roles = items.at(i).set_status();
            if (roles.size())
            {
                auto idx = index(i);
                emit dataChanged(idx, idx, roles);
            }
            break;
        }
}

void SectionsModel::newValue(Device_Item* /*item*/)
{
    QVector<int> roles;
    auto group = static_cast<Device_item_Group*>(sender());
    for (uint i = 0; i < items.size(); i++)
        if (items.at(i).group == group)
        {
            for (Device_Item* item: group->items())
            {
                roles.clear();

                if (!items.at(i).devDevice_item_type) {
                    items.at(i).devDevice_item_type = item->type_id();
                    roles.push_back(SignRole);
                }

                double val = 0;
                if (val != items.at(i).value)
                {
                    items.at(i).value = val;
                    roles.push_back(ValueRole);
                }

                if (roles.size()) {
                    auto idx = index(i);
                    emit dataChanged(idx, idx, roles);
                }
                return;
            }
        }
}

} // namespace Gui
} // namespace Das
