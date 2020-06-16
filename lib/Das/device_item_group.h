#ifndef DAS_DEVICE_ITEM_GROUP_H
#define DAS_DEVICE_ITEM_GROUP_H

#include <QMutex>

#include <memory>
#include <set>

#include <Das/db/device_item_group.h>
#include <Das/db/dig_status.h>
#include <Das/db/dig_mode.h>
#include <Das/param/paramgroup.h>
#include <Das/device_item.h>

namespace Das {
namespace DB {
class DIG_Type;
} // namespace DB
class Section;

class DAS_LIBRARY_SHARED_EXPORT Device_item_GroupStatus : public QObject
{
    Q_OBJECT
public:
    uint32_t info_id_;
    QStringList args_;
};

class DAS_LIBRARY_SHARED_EXPORT Device_item_Group : public QObject, public DB::Device_Item_Group
{
    Q_OBJECT
    Q_PROPERTY(uint32_t id READ id)
    Q_PROPERTY(uint32_t section_id READ section_id)
    Q_PROPERTY(uint32_t type READ type_id)
    Q_PROPERTY(uint32_t mode READ mode_id WRITE set_mode NOTIFY mode_changed)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(Device_Items items READ items)
    Q_PROPERTY(Param* param READ params)
    Q_PROPERTY(Section* section READ section)
public:
    Device_item_Group(uint32_t id = 0, const QString& title = {}, uint32_t section_id = 0, uint32_t type_id = 0, uint32_t mode_id = 0);
    Device_item_Group(DB::Device_Item_Group&& o, uint32_t mode_id);
    Device_item_Group(Device_item_Group&& o);
    Device_item_Group(const Device_item_Group& o);
    Device_item_Group& operator =(Device_item_Group&& o);
    Device_item_Group& operator =(const Device_item_Group& o);

    QString name() const;

    uint32_t mode_id() const;
    const DIG_Mode& mode_data() const;

    Section* section() const;
    void set_section(Section* section);

    const Device_Items& items() const;
    Param* params();
    const Param* params() const;
    void set_params(std::shared_ptr<Param> param_group);

    void add_item(Device_Item* item);
    void remove_item(Device_Item* item);
    void finalize();

    const std::set<DIG_Status> &get_statuses() const;
    void set_statuses(const std::set<DIG_Status>& statuses);
signals:

    void item_changed(Device_Item*, uint32_t user_id = 0, const QVariant& old_raw_value = QVariant());
    void control_changed(Device_Item*, uint32_t user_id = 0, const QVariant& old_raw_value = QVariant());
    void sensor_changed(Device_Item*, uint32_t user_id = 0, const QVariant& old_raw_value = QVariant());

    void control_state_changed(Device_Item* item, const QVariant& raw_data, uint32_t user_id = 0);

    void param_changed(Das::Param* param, uint32_t user_id = 0);
    void mode_changed(uint32_t user_id, uint32_t mode_id, uint32_t group_id);
    void status_changed(const DIG_Status& status);

    void connection_state_change(Device_Item*, bool value);

public slots:
    QString toString() const;

    void set_mode(uint32_t mode_id, uint32_t user_id = 0, qint64 timestamp_msec = DB::Log_Base_Item::current_timestamp());

    const std::set<DIG_Status>& statuses() const;

    bool check_status(uint32_t status_id) const;
    void add_status(uint32_t status_id, const QStringList& args = QStringList(), uint32_t user_id = 0);
    void remove_status(uint32_t status_id, uint32_t user_id = 0);
    void clear_status(uint32_t user_id = 0);

    bool write_to_control(uint32_t type_id, const QVariant& raw_data, uint32_t mode_id = 0, uint32_t user_id = 0);
    bool write_to_control(Device_Item* item, const QVariant& raw_data, uint32_t mode_id = 0, uint32_t user_id = 0);

    Device_Item* item_by_type(uint type_id) const;
    QVector<Device_Item*> items_by_type(uint type_id) const;

private slots:
    void value_changed(uint32_t user_id, const QVariant& old_raw_value);
    void connection_state_changed(bool value);
private:


    DIG_Mode mode_;

    Section* sct_;
    DB::DIG_Type* type_;

    Device_Items items_;
    std::shared_ptr<Param> param_group_;

    std::set<DIG_Status> statuses_;
};

QDataStream &operator<<(QDataStream& ds, Device_item_Group* group);

typedef std::shared_ptr<Device_item_Group> Device_item_GroupPtr;

} // namespace Das

#endif // DAS_DEVICE_ITEM_GROUP_H
