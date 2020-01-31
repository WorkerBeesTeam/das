#ifndef DAS_CHECKER_INTERFACE_H
#define DAS_CHECKER_INTERFACE_H

#include <QtPlugin>

QT_FORWARD_DECLARE_CLASS(QSettings)

#include <Das/write_cache_item.h>

namespace Das {

class Device;
class Scheme;

class Checker_Interface
{
public:
    virtual ~Checker_Interface() = default;
    virtual void configure(QSettings* settings, Scheme* prj) = 0;

    virtual bool check(Device *dev) = 0;
    virtual void stop() = 0;

    virtual void write(std::vector<Write_Cache_Item>& items) = 0;
};

} // namespace Das

#define DasCheckerInterface_iid "ru.deviceaccess.Das.Checker_Interface"
Q_DECLARE_INTERFACE(Das::Checker_Interface, DasCheckerInterface_iid)

#endif // DAS_CHECKERINTERFACE_H
