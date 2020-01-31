#ifndef DAS_SCHEME_H
#define DAS_SCHEME_H

#include <QLoggingCategory>

#include <Das/db/dig_param_value.h>
#include "type_managers.h"
#include "section.h"

namespace Das
{

Q_DECLARE_LOGGING_CATEGORY(SchemeLog)
Q_DECLARE_LOGGING_CATEGORY(SchemeDetailLog)

class DAS_LIBRARY_SHARED_EXPORT Scheme : public QObject, public Type_Managers
{
    Q_OBJECT
    Q_PROPERTY(Sections sections READ sections)
    Q_PROPERTY(Devices devices READ devices)
public:
    Scheme(QObject* parent = nullptr);
    ~Scheme();

    /*static void fillItemTypes(ItemTypeManager* mng, const Prt::TypesInfo *info);
    static void fillGroupTypes(Device_item_GroupTypeManager* mng, const Prt::TypesInfo *info);
    static void fillParamTypes(ParamTypeManager* mng, const Prt::TypesInfo *info);
    static void fillCodes(CodeManager* mng, const Prt::TypesInfo *info);
    static void fillStatusTypes(StatusTypeManager* mng, const Prt::TypesInfo *info);
    static void fillStatuses(StatusManager* mng, const Prt::TypesInfo *info);

    void initTypes(const Prt::TypesInfo &typeInfo);*/
//    void init(const Prt::ServerInfo& info);
//    void setData(Database* db, const Prt::ServerInfo& info);

    Q_INVOKABLE const Devices& devices() const;
    Q_INVOKABLE const Sections& sections() const;

    Q_INVOKABLE Section* tambour();

    static void sort_devices(Devices &devices);
    void sort_devices();

    void set_devices(const Devices& devices);
    void clear_devices();

    uint section_count();
    void clear_sections();

    virtual Section *add_section(Section&& section);
    virtual Device *add_device(Device&& device);

    Device* dev_by_id(uint32_t id) const;
    Section* section_by_id(uint32_t id) const;
    Device_Item *item_by_id(uint32_t id) const;
//    Device_item_Group* group_by_id(uint32_t id) const;
signals:
    void control_state_changed(Device_Item* item, const QVariant& raw_data, uint32_t user_id = 0);
    void mode_changed(uint32_t user_id, uint mode_id, uint32_t group_id);
public slots:
    void set_mode(uint32_t user_id, uint32_t mode_id, uint32_t group_id);

    void set_dig_param_values(uint32_t user_id, QVector<DIG_Param_Value> params);
private:
    template<class T>
    T* by_id(uint32_t id, const QVector< T* >& items) const;

    Devices devs_;
    Sections scts_;
};

} // namespace Das

#endif // DAS_SCHEME_H
