#ifndef DAS_WIRINGPIPLUGIN_H
#define DAS_WIRINGPIPLUGIN_H

#include <memory>

#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checkerinterface.h>

namespace Das {
namespace WiringPi {

Q_DECLARE_LOGGING_CATEGORY(WiringPiLog)

class DAS_PLUGIN_SHARED_EXPORT WiringPiPlugin : public QObject, public Checker_Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker_Interface)

public:
    WiringPiPlugin();
    ~WiringPiPlugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings, Scheme*) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
};

} // namespace WiringPi
} // namespace Das

#endif // DAS_WIRINGPIPLUGIN_H
