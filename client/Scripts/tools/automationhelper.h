#ifndef AUTOMATIONHELPER_H
#define AUTOMATIONHELPER_H

#include <vector>

#include <QScriptValue>

#include <Das/section.h>

namespace Das {

class AutomationHelperItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(Device_item_Group* group READ group)
    Q_PROPERTY(uint type READ type WRITE setType)
    Q_PROPERTY(QScriptValue data READ data WRITE setData)
public:
    AutomationHelperItem(Device_item_Group* group = nullptr, uint type = 0, const QScriptValue &value = QScriptValue(), QObject* parent = nullptr);

    Device_item_Group* group() const;
    uint type() const;

    QScriptValue data() const;
    void setData(const QScriptValue &value);

public slots:
    virtual void init() {}
    void setType(uint type);
    void writeToControl(const QVariant &raw_data, uint mode = 2);
private:
    Device_item_Group* m_group;
    uint m_type;
    QScriptValue m_data;

    friend class AutomationHelper;
};

class AutomationHelper : public QObject {
    Q_OBJECT
public:
    AutomationHelper(const QScriptValue &type = QScriptValue(), QObject* parent = nullptr);
    void clear();
public slots:
    void init();
    void addGroup(AutomationHelperItem *helper);
    void addGroup(Device_item_Group* group, const QScriptValue &value);
    AutomationHelperItem* item(Device_item_Group* group) const;
    AutomationHelperItem* item(uint group_id) const;
private:
    std::vector<AutomationHelperItem*> m_items;
    uint m_type;
};

} // namespace Das

Q_DECLARE_METATYPE(Das::AutomationHelper*)
Q_DECLARE_METATYPE(Das::AutomationHelperItem*)

#endif // AUTOMATIONHELPER_H
