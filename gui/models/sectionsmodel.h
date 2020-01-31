#ifndef DAS_GUI_SECTIONSMODEL_H
#define DAS_GUI_SECTIONSMODEL_H

#include <QQmlEngine>
#include <QQuickItem>

#include "templatemodel.h"

namespace Das {
namespace Gui {

class GroupStatusInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool inform READ inform)
    Q_PROPERTY(uint32_t status READ status)
    Q_PROPERTY(QString color READ color)
    Q_PROPERTY(QStringList descriptions READ descriptions)
public:
    GroupStatusInfo(bool inform = false, uint32_t status = 0, QString color = QString(), QStringList descriptions = QStringList());
    GroupStatusInfo(GroupStatusInfo&& o);
//    GroupStatusInfo(const GroupStatusInfo&) = default;

    bool inform() const;
    uint32_t status() const;
    const QString& color() const;
    const QStringList& descriptions() const;
private:
    bool m_inform;
    uint32_t m_status;
    QString m_color;
    QStringList m_descriptions;
};

class SectionsModel;
struct SectionModelItem : public TemplateItem {
    SectionModelItem(Section* section, Device_item_Group* groupPtr);
    SectionModelItem(const SectionModelItem&) = default;

    QVector<int> set_status();


    GroupStatusInfo getStatusInfo() const;

    Device_item_Group* group;
    double value;
    QColor status_color;
    uint32_t devDevice_item_type = 0;
    QStringList status_text;
};

class SectionsModel : public TemplateModel<SectionsModel, SectionModelItem>
{
    Q_OBJECT
public:
    enum SectionRoles {
        StatusColorRole = UserRolesCount,
        StatusTextRole,
        ValueColorRole,
        SignRole,
        TypeRole,
        ParamRole,
    };

    SectionsModel();
    void reset();

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap getComboParamValues(Param* combo_param) const {
        if (!combo_param || combo_param->type()->value_type() != DIG_Param_Type::VT_COMBO)
            return {};

        QVariantMap combo_values;
        for (const DIG_Param_Type& param: prj()->param_mng_.types()) {
            if (param.parent_id == combo_param->type()->id())
                combo_values[ param.description().isEmpty() ? param.title() : param.description() ] = param.title();
        }
        return combo_values;
    }
private slots:
    void groupStatusChanged(uint32_t status);
private:
    void newValue(Device_Item *item);

    QVariant data(const SectionModelItem& item, int role) const override;
    void setData(SectionModelItem& item, const QVariant &value, int role, int& ok_role) override;
};

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_SECTIONSMODEL_H
