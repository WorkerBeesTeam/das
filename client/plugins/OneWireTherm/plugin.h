#ifndef DAS_ONEWIRETHERMPLUGIN_H
#define DAS_ONEWIRETHERMPLUGIN_H

#include <memory>

#include <QFile>

#include <Helpz/simplethread.h>

#include <Das/checker_interface.h>

#include "../plugin_global.h"

#include "one_wire_therm_task.h"

namespace Das {
namespace OneWireTherm {

class DAS_PLUGIN_SHARED_EXPORT OneWireThermPlugin : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)

public:
    OneWireThermPlugin();
    ~OneWireThermPlugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
private:
    using Therm_Task_Thread = Helpz::ParamThread<One_Wire_Therm_Task>;
    Therm_Task_Thread* therm_thread_ = nullptr;
};

} // namespace OneWireTherm
} // namespace Das

#endif // DAS_ONEWIRETHERMPLUGIN_H
