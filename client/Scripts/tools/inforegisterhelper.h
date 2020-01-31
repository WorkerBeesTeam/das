#ifndef INFOREGISTERHELPER_H
#define INFOREGISTERHELPER_H

#include "automationhelper.h"

namespace Das {

class InfoRegisterHelper : public AutomationHelperItem
{
    Q_OBJECT
    Q_PROPERTY(int size READ size)
public:
    explicit InfoRegisterHelper(Device_item_Group* group, uint type, uint info_type, QObject *parent = 0);

    int size() const;
public slots:
    void init(InfoRegisterHelper* other = nullptr);
    QVariant info(Device_Item* item) const;

    Device_Item* wndItem(uint i) const;
    Device_Item* infoItem(uint i) const;
private:
    Device_Item* getItem(uint i, bool getWnd = true) const;

    uint m_info_type;
    std::map<Device_Item*, Device_Item*> m_items;
};

} // namespace Das

#endif // INFOREGISTERHELPER_H
