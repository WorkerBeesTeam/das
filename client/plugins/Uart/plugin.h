#ifndef DAS_UART_PLUGIN_H
#define DAS_UART_PLUGIN_H

#include <QLoggingCategory>

#include <memory>
#include <set>

#include "../plugin_global.h"
#include <Das/checkerinterface.h>

#include "config.h"

namespace Das {
Q_DECLARE_LOGGING_CATEGORY(UartLog)

class DAS_PLUGIN_SHARED_EXPORT Uart_Plugin : public QObject, public Checker_Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker_Interface)
public:
    Uart_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings, Scheme* scheme) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
private:
    bool is_port_name_in_config_;
    Uart::Config config_;
    QSerialPort port_;
};

} // namespace Das

#endif // DAS_UART_PLUGIN_H
