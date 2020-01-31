#ifndef DAS_GUI_EQUIPMENTMODEL_H
#define DAS_GUI_EQUIPMENTMODEL_H

#include "templatemodel.h"

namespace Das {
namespace Gui {

class EquipmentGroupModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        ValueRole,
        IsCoilsRole
    };

    EquipmentGroupModel() = default;
    EquipmentGroupModel(Device_item_Group* group);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &p = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) override;
private slots:
    void changed(Device_Item *item);
private:
    Device_Items items;
};

// ----------------------------------------------------------------------------------------------------

struct EquipmentItem : public TemplateItem {
    EquipmentItem(Device_item_Group* group) :
        TemplateItem(group->section()), model(new EquipmentGroupModel(group)), group(group) {}
    EquipmentItem(const EquipmentItem& other) :
        TemplateItem(other.sct), model(new EquipmentGroupModel(other.group)), group(other.group) {}
    EquipmentItem(EquipmentItem&& other) :
        TemplateItem(other.sct) {
        this->model = std::move(other.model);
        this->group = other.group;
        other.model = nullptr;
    }

    ~EquipmentItem() {
        delete model;
    }

    EquipmentGroupModel* model = nullptr;
    Device_item_Group* group;
};

class EquipmentModel : public TemplateModel<EquipmentModel, EquipmentItem>
{
    Q_OBJECT
public:
    enum Roles {
        ModelRole = UserRolesCount,
        ModeRole,
    };

    EquipmentModel();
    void reset() override;

    QHash<int, QByteArray> roleNames() const override;
protected:
    QVariant data(const EquipmentItem& item, int role) const override;
    void setData(EquipmentItem& item, const QVariant &value, int role, int& ok_role) override;
private slots:
    void modeChanged(uint);
};

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_EQUIPMENTMODEL_H
