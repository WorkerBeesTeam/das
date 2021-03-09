#include <QDebug>

#include <memory>

#include <Helpz/apply_parse.h>

#include "param/paramgroup.h"
#include "device.h"
#include "scheme.h"

namespace Das
{

Q_LOGGING_CATEGORY(SchemeLog, "scheme")
Q_LOGGING_CATEGORY(SchemeDetailLog, "scheme.detail", QtInfoMsg)

Scheme::Scheme(/*Database *db, */QObject *parent) :
    QObject(parent)/*, db_(db)*/
{
    qRegisterMetaType<QVector<DB::DIG_Param_Value_Base>>("QVector<DB::DIG_Param_Value_Base>");
    qRegisterMetaType<QVector<DIG_Param_Value>>("QVector<DIG_Param_Value>");
    qRegisterMetaType<Log_Value_Item>("Log_Value_Item");

    plugin_type_mng_ = std::make_shared<Plugin_Type_Manager>();
}

Scheme::~Scheme()
{
    clear_devices();
    clear_sections();
}

const Devices &Scheme::devices() const { return devs_; }
const Sections &Scheme::sections() const { return scts_; }

Section *Scheme::tambour() { return scts_.size() ? scts_.first() : nullptr; }

/*static*/ void Scheme::sort_devices(Devices &devices)
{
    std::sort(devices.begin(), devices.end(), [](Device* dev1, Device* dev2) -> bool
    {
        if (dev1->items().size() && dev2->items().size())
        {
            Device_Item* dev1it = dev1->items().first();
            Device_Item* dev2it = dev2->items().first();

            if (dev1it->register_type() != dev2it->register_type())
                return dev1it->register_type() < dev2it->register_type();

            return dev1it->type_id() < dev2it->type_id();
        }
        else
            return false;
    });
}

void Scheme::sort_devices() { sort_devices(devs_); }

void Scheme::set_devices(const Devices &devices)
{
    clear_devices();
    devs_ = devices;
    sort_devices(devs_);
}

void Scheme::clear_devices() { item_delete_later(devs_); }
void Scheme::clear_sections() { item_delete_later(scts_); }
uint Scheme::section_count() { return scts_.size(); }

Section *Scheme::add_section(Section&& section)
{
    SectionPtr sct = new Section{ std::move(section) };
    sct->set_type_managers(this);

    connect(sct, &Section::control_state_changed, this, &Scheme::control_state_changed);
    connect(sct, &Section::mode_changed, this, &Scheme::mode_changed);

    scts_.push_back( sct );
    return sct;
}

Device *Scheme::add_device(Device&& device)
{
    Device* dev = new Device{ std::move(device) };
    dev->set_scheme(this);
    devs_.push_back(dev);
    return dev;
}

void Scheme::set_mode(uint32_t user_id, uint32_t mode_id, uint32_t group_id)
{
    Device_item_Group* group;
    for (Section* sct: scts_)
        if (group = sct->group_by_id(group_id), group)
        {
            group->set_mode(mode_id, user_id);
            break;
        }
}

void Scheme::set_dig_param_values(uint32_t user_id, const QVector<DB::DIG_Param_Value_Base> &params)
{
    if (params.empty())
        return;

    auto dbg = qDebug(SchemeLog).nospace().noquote() << user_id << "|Params changed:";

    for (const DB::DIG_Param_Value_Base& item: params)
    {
        Param* p = dig_param_by_id(item.group_param_id());
        if (p)
        {
            dbg << '\n' << item.group_param_id() << ' ' << p->toString() << ": \"" << item.value().left(128) << "\"";
            p->set_value_from_string(item.value(), user_id);
        }
        else
            qCWarning(SchemeLog) << "Can't find dig param with id:" << item.group_param_id();
    }
}

Device *Scheme::dev_by_id(uint32_t id) const { return by_id(id, devs_); }
Section* Scheme::section_by_id(uint32_t id) const { return by_id(id, scts_); }
Device_Item* Scheme::item_by_id(uint32_t id) const
{
    for (Device* dev: devs_)
        for (Device_Item* item: dev->items())
            if (item->id() == id)
                return item;
    return nullptr;
}

Param *Scheme::dig_param_by_id(uint32_t id) const
{
    Param* p;
    for(Section* sct: scts_)
        for (Device_item_Group* group: sct->groups())
            if (p = group->params()->get_by_id(id), p)
                return p;
    return nullptr;
}

template<class T>
T* Scheme::by_id(uint32_t id, const QVector<T*> &items) const
{
    for (auto&& item: items)
        if (item->id() == id)
            return item;
    return nullptr;
}

} // namespace Das
