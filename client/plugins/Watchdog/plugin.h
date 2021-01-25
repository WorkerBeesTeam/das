#ifndef DAS_RANDOMPLUGIN_H
#define DAS_RANDOMPLUGIN_H

#include <QLoggingCategory>

#include <memory>
#include <set>

#include <Helpz/zfile.h>

#include "../plugin_global.h"
#include <Das/checker_interface.h>

namespace Das {
namespace Watchdog {

Q_DECLARE_LOGGING_CATEGORY(Log)

class DAS_PLUGIN_SHARED_EXPORT Plugin : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)
public:
    Plugin();
    ~Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override final;
    void write(std::vector<Write_Cache_Item>& items) override;

private:
    void open_device(int interval_sec);
    void reset_timer();
    QVariant get_reset_cause();

    bool _is_error = false, _stop_at_exit;
    int _max_interval;
    Helpz::File _file;
    QVariant _reset_cause;
};

} // namespace Watchdog
} // namespace Das

#endif // DAS_RANDOMPLUGIN_H
