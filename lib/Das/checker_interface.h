#ifndef DAS_CHECKER_INTERFACE_H
#define DAS_CHECKER_INTERFACE_H

#include <QtPlugin>

QT_FORWARD_DECLARE_CLASS(QSettings)

#include <Das/write_cache_item.h>

namespace Das {

class Device;
class Scheme;

namespace Checker {

class Interface;
class Manager_Interface
{
public:
    virtual ~Manager_Interface() = default;

    virtual void send_stream_toggled(uint32_t user_id, Device_Item* item, bool state) = 0;
    virtual void send_stream_param(Device_Item* item, const QByteArray& data) = 0;
    virtual void send_stream_data(Device_Item* item, const QByteArray& data) = 0;
protected:
    void init_checker(Interface* checker, Scheme* scheme);
};

class Interface
{
public:
    Interface();
    virtual ~Interface() = default;
    virtual void configure(QSettings* settings) = 0;

    virtual bool is_need_own_thread() const { return false; }
    virtual bool check(Device *dev) = 0;
    virtual void stop() = 0;

    virtual void write(std::vector<Write_Cache_Item>& items) = 0;

    virtual void toggle_stream(uint32_t user_id, Device_Item* item, bool state)
    {
        Q_UNUSED(user_id);
        Q_UNUSED(item);
        Q_UNUSED(state);
    }

    Manager_Interface* manager();
    Scheme* scheme();

private:
    Manager_Interface* mng_;
    Scheme* scheme_;

    friend class Manager_Interface;
};

} // namespace Checker
} // namespace Das

#define DasCheckerInterface_iid "ru.deviceaccess.Das.Checker_Interface"
Q_DECLARE_INTERFACE(Das::Checker::Interface, DasCheckerInterface_iid)

#endif // DAS_CHECKERINTERFACE_H
