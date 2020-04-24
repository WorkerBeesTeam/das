#ifndef DAS_PARAMGROUP_H
#define DAS_PARAMGROUP_H

#include <QVariant>

#include <memory>
#include <vector>

#include <Das/db/dig_param_type.h>
#include <Das/db/dig_param.h>

namespace Das {

class Device_item_Group;

class DAS_LIBRARY_SHARED_EXPORT Param : public QObject {
    Q_OBJECT
    Q_PROPERTY(DIG_Param_Type* type READ type)
    Q_PROPERTY(uint32_t id READ id)
    Q_PROPERTY(QVariant value READ value WRITE set_value NOTIFY value_changed)
public:
    Param(Device_item_Group* owner = nullptr);
    Param(uint id, DIG_Param_Type* param, const QString& raw_value, Device_item_Group* owner = nullptr);
    Param(const QVariant& value, uint id, DIG_Param_Type* param, Device_item_Group* owner = nullptr);

    Device_item_Group *group() const;
    void set_group(Device_item_Group *group);

    static QVariant value_from_string(DIG_Param_Type::Value_Type value_type, const QString& raw_value);
    static QString value_to_string(DIG_Param_Type::Value_Type value_type, const QVariant &value);

    void set_value(const QVariant& param_value, uint32_t user_id = 0);
    Q_INVOKABLE void set_value_from_string(const QString& raw_value, uint32_t user_id = 0);
    Q_INVOKABLE QString value_to_string() const;

    Q_INVOKABLE std::size_t count() const;

    uint id() const;
    DIG_Param_Type* type() const;
    const QVariant& value() const;

    bool add(const DIG_Param &dig_param, const QString &value_string, DIG_Param_Type_Manager* mng);
    bool add(const std::shared_ptr<Param>& elem);

    Q_INVOKABLE bool has(const QString& name) const;
    Q_INVOKABLE Param* get(const QString& name) const;
    Q_INVOKABLE Param* get(uint index) const;

    Q_INVOKABLE Param* get_by_id(uint id) const;
    Q_INVOKABLE Param* get_by_type_id(uint type_id) const;
signals:
    void value_changed();
public slots:
    QString toString() const;
private:
    std::vector<std::shared_ptr<Param>>::const_iterator get_iterator(const QString& name) const;
    void add_child(const std::shared_ptr<Param>& elem);

    uint id_ = 0;
    DIG_Param_Type* type_ = nullptr;
    Device_item_Group* group_;
    QVariant value_;

    Param* parent_;
    std::vector<std::shared_ptr<Param>> childrens_;

    static DIG_Param_Type empty_type;
};

} // namespace Das

//Q_DECLARE_METATYPE(Das::Param)

Q_DECLARE_METATYPE(Das::Param*)

#endif // DAS_PARAMGROUP_H
