#ifndef DAS_CHECKER_H
#define DAS_CHECKER_H

#include <QThread>
#include <QTimer>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QPluginLoader>

#include <map>

#include <Helpz/simplethread.h>

//#include <Das/scheme.h>
#include <Das/checker_interface.h>
#include <Das/write_cache_item.h>

namespace Das {

namespace DB {
class Plugin_Type;
class Plugin_Type_Manager;
} // namespace DB

class Scripted_Scheme;
class Worker;
typedef std::map<Device_Item*, QVariant> ChangesList;

namespace Checker {

Q_DECLARE_LOGGING_CATEGORY(Log)

class Manager : public QObject, public Manager_Interface
{
    Q_OBJECT
public:
    explicit Manager(Worker* worker, QObject *parent = 0);
    ~Manager();

    void break_checking();
public slots:
    void stop();
    void start();
    std::future<QByteArray> start_stream(uint32_t user_id, Device_Item* item, const QString &url);

    void set_custom_check_interval(Device* dev, uint32_t interval);
private slots:
    void start_devices();
    void check_devices();
    void write_data(Device_Item* item, const QVariant& raw_data, uint32_t user_id = 0);
    void write_cache();
private:
    void write_items(Device* dev, std::vector<Write_Cache_Item>& items);

    void load_plugins();
    DB::Plugin_Type* load_plugin(const QString& file_path, std::shared_ptr<QPluginLoader>& loader);

    bool is_server_connected() const override;

    uint32_t get_device_check_interval(Device* dev) const;

    bool b_break;

    QTimer check_timer_, write_timer_;

    Worker* worker_;
    Scripted_Scheme* scheme_;
    std::map<Device*, std::vector<Write_Cache_Item>> write_cache_;

    std::map<Device*, uint32_t> _custom_check_interval;

    std::shared_ptr<DB::Plugin_Type_Manager> plugin_type_mng_;

    struct Check_Info
    {
        bool status_;
        qint64 time_;
    };
    QMap<uint32_t, Check_Info> last_check_time_map_;
};

} // namespace Checker
} // namespace Das

#endif // DAS_CHECKER_H
