#ifndef DAS_GUI_SENSORMODEL_H
#define DAS_GUI_SENSORMODEL_H

#include "templatemodel.h"

namespace Das {
namespace Gui {

struct SensorItem : public TemplateItem {
    SensorItem(Section* section, Device_Item* item, Device_item_Group* group) :
        TemplateItem(section), item(item), group(group) {}
    Device_Item* item;
    Device_item_Group* group;
};

class SensorModel : public TemplateModel<SensorModel, SensorItem>
{
    Q_OBJECT
public:
    enum SensorRoles {
        SignRole = UserRolesCount,
    };

    SensorModel();
    void reset();
private slots:
    void changes(uint32_t user_id);
private:
    QVariant data(const SensorItem& item, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant valueOfType(Device_Item* item) const;
};

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_SENSORMODEL_H
