#ifndef DAS_SECTION_H
#define DAS_SECTION_H

#include <QObject>
#include <QPointer>
#include <QVector>

#include <Das/timerange.h>
#include <Das/device_item_group.h>
#include <Das/type_managers.h>

namespace Das
{

class DAS_LIBRARY_SHARED_EXPORT Section : public QObject, public DB::Base_Type
{
    Q_OBJECT
    Q_PROPERTY(uint32_t id READ id WRITE set_id)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(TimeRange* dayTime READ day_time)
    Q_PROPERTY(Device_Items items READ items)

    HELPZ_DB_META(Section, "section", "s", DB_A(id), DB_A(name), DB_A(day_start), DB_A(day_end), DB_A(scheme_id))
public:
    explicit Section(uint32_t id = 0, const QString& name = {}, uint32_t day_start_secs = 0, uint32_t day_end_secs = 0);
    Section(Section&& o);
    Section(const Section& o);
    Section& operator =(Section&& o);
    Section& operator =(const Section& o);
    ~Section();
//    Section(const Prt::Section& sct, Type_Managers* typeMng, QObject *parent = 0);

    uint32_t day_start() const { return day_.start(); }
    void set_day_start(uint32_t dayStart) { day_.set_start(dayStart); }

    uint32_t day_end() const { return day_.end(); }
    void set_day_end(uint32_t dayEnd) { day_.set_end(dayEnd); }

    TimeRange *day_time();
    const TimeRange *day_time() const;

    enum ExtraState {
        xError           = 8,
        xExecuting       = 16,
    };

    Device_item_Group* add_group(DB::Device_Item_Group&& grp, uint32_t mode_id);

    Q_INVOKABLE QVector<Device_item_Group*> groups_by_dev_Device_item_type(uint device_item_type_id) const;
    Q_INVOKABLE Device_item_Group* group_by_dev_Device_item_type(uint device_item_type_id) const;
    Q_INVOKABLE Device_item_Group* group_by_type(uint g_type) const;
    Q_INVOKABLE QVector<Device_item_Group*> groups_by_type(uint g_type) const;
    Q_INVOKABLE Device_item_Group* group_by_id(uint g_id) const;
    Q_INVOKABLE QVector<Device_Item*> items_by_type(uint device_item_type_id, uint32_t group_type_id = 0) const;
    Q_INVOKABLE Device_Item* item_by_type(uint device_item_type_id, uint32_t group_type_id = 0) const;

    Device_Items items() const;

    Type_Managers *type_managers() const;
    void set_type_managers(Type_Managers *type_mng);

    const QVector<Device_item_Group*>& groups() const;

signals:
    void group_initialized(Device_item_Group*);

    void item_changed(Device_Item*, uint32_t user_id = 0, const QVariant& old_raw_value = QVariant());
    void control_changed(Device_Item*, uint32_t user_id = 0, const QVariant& old_raw_value = QVariant());
//    void sensorChanged(Device_Item*);

    void control_state_changed(Device_Item* item, const QVariant& raw_data, uint32_t user_id = 0); // deprecated
    void mode_changed(uint32_t user_id, uint32_t mode_id, uint32_t group_id);

//    bool checkValue(Device_item_Group* group, const Device_Item::ValueType& val) const;
//    uint32_t getGroupStatus(Device_item_Group* group, Device_item_Group::ValueType val) const;
public slots:
    QString toString() const;

    void write_to_control(uint type, const QVariant &data, uint32_t mode = 0, uint32_t user_id = 0);
private:
    TimeRange day_;
    Type_Managers* type_mng_ = nullptr;

    QVector<Device_item_Group*> groups_;

    friend QDataStream &operator>>(QDataStream &s, Section &sct);
};

QDataStream &operator>>(QDataStream& ds, Section &sct);
QDataStream &operator<<(QDataStream& ds, const Section &sct);
QDataStream &operator<<(QDataStream& ds, Section* sct);

//typedef std::shared_ptr<Section> SectionPtr;
typedef Section* SectionPtr;
typedef QVector< SectionPtr > Sections;

} // namespace Das

Q_DECLARE_METATYPE(Das::Sections)

#endif // DAS_SECTION_H
