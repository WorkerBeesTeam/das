#ifndef DAS_WATCHDOG_PLUGIN_H
#define DAS_WATCHDOG_PLUGIN_H

#include <QLoggingCategory>

#include <memory>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <Helpz/zfile.h>

#include "../plugin_global.h"
#include <Das/checker_interface.h>

namespace Das {
namespace Watchdog {

Q_DECLARE_LOGGING_CATEGORY(Log)

class DAS_PLUGIN_SHARED_EXPORT Plugin final : public QObject, public Checker::Interface
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
    void write(Device* dev, std::vector<Write_Cache_Item>& items) override;

private:
    void open_device(int interval_sec);
    void reset_timer();
    QVariant get_reset_cause();

    void send_reset_cause(Device_Item* dev_item);
    void send_reset_cause(Device_Item* dev_item, qint64 ts, const QVariant& value);

    void print_reset_cause() const;
    QString get_uptime() const;
    QString reset_cause_str() const;

    void run(const std::string& dev_file_name);

    void process();
    void close_device();

    bool _is_first_call = true;
    bool _is_error = false, _stop_at_exit;
    int _max_interval;
    Helpz::File _file;
    QVariant _reset_cause;

    bool _stop_flag = false;
    std::mutex _mutex;
    std::thread _thread;
    std::condition_variable _cond;
    std::chrono::milliseconds _user_timeout;
    std::chrono::milliseconds _real_interval;
    std::chrono::system_clock::time_point _last_check;
};

} // namespace Watchdog
} // namespace Das

#endif // DAS_WATCHDOG_PLUGIN_H
