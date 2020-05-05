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
    bool is_counter_result_;
    bool is_zero_disconnected_;
    int sleep_ms_;
    int max_count_;
    int sum_for_;
    int devide_by_;
};

class DAS_PLUGIN_SHARED_EXPORT ResistancePlugin : public QObject, public Checker::Interface
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
    void write(std::vector<Write_Cache_Item>& items) override;

private:
    void run();
    int read_item(int pin);

    Config conf_;

    std::thread thread_;
    std::mutex mutex_;
    bool break_;
    std::condition_variable cond_;

    std::map<int, Device_Item*> data_;
};

} // namespace Resistance
} // namespace Das

#endif // DAS_RESISTANCE_PLUGIN_H
