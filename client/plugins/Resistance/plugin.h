#ifndef DAS_RESISTANCE_PLUGIN_H
#define DAS_RESISTANCE_PLUGIN_H

#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checker_interface.h>

namespace Das {
namespace Resistance {

Q_DECLARE_LOGGING_CATEGORY(ResistanceLog)

struct Config
{
    bool is_zero_disconnected_;
    int sleep_ms_;
    int max_count_;
    int sum_for_;
    int devide_by_;
};

class DAS_PLUGIN_SHARED_EXPORT ResistancePlugin final : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)

public:
    ResistancePlugin();
    ~ResistancePlugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(Device* dev, std::vector<Write_Cache_Item>& items) override;

private:
    void run();
    int read_item(int pin);

    Config _conf;

    std::thread _thread;
    std::mutex _mutex;
    bool _break;
    std::condition_variable _cond;

    std::map<int, Device_Item*> _data;
};

} // namespace Resistance
} // namespace Das

#endif // DAS_RESISTANCE_PLUGIN_H
