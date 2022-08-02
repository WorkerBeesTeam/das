#ifndef DAS_CHECKER_INTERFACE_H
#define DAS_CHECKER_INTERFACE_H

#include <future>

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

    virtual bool is_server_connected() const = 0;
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
    virtual void start() {}
    virtual bool check(Device *dev) = 0;
    virtual void stop() {}

    virtual void write(Device* dev, std::vector<Write_Cache_Item>& items) = 0;

    virtual std::future<QByteArray> start_stream(uint32_t user_id, Device_Item* item, const QString &url)
    {
        // This funciton can call from another thread
        Q_UNUSED(user_id);
        Q_UNUSED(item);
        Q_UNUSED(url);
        return {};
    }

    Manager_Interface* manager();
    const Manager_Interface* manager() const;
    Scheme* scheme();
    const Scheme* scheme() const;

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
