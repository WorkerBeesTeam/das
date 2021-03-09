#ifndef DAS_UART_PLUGIN_H
#define DAS_UART_PLUGIN_H

#include "../plugin_global.h"
#include <Das/checker_interface.h>

#include "uart_thread.h"

namespace Das {

class DAS_PLUGIN_SHARED_EXPORT Uart_Plugin final : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)
public:
    Uart_Plugin();
    ~Uart_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(Device* dev, std::vector<Write_Cache_Item>& items) override;
private:
    Uart_Thread _thread;
};

} // namespace Das

#endif // DAS_UART_PLUGIN_H
