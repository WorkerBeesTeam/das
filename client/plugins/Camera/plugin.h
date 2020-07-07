#ifndef DAS_CAMERA_PLUGIN_H
#define DAS_CAMERA_PLUGIN_H

#include <memory>
#include <set>
#include <functional>

#include <QThread>

#include "../plugin_global.h"
#include <Das/device.h>

#include "camera_thread.h"

namespace Das {

class DAS_PLUGIN_SHARED_EXPORT Camera_Plugin : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)
public:
    Camera_Plugin();
    ~Camera_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
    void toggle_stream(uint32_t user_id, Device_Item *item, bool state) override;
public slots:
    void save_frame(Device_Item* item);
private:
    Camera_Thread thread_;
};

} // namespace Das

#endif // DAS_UART_PLUGIN_H
