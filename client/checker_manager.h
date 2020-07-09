#ifndef DAS_CHECKER_H
#define DAS_CHECKER_H

#include <QThread>
#include <QTimer>
#include <QLoggingCategory>
#include <QMutexLocker>

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
    void loadPlugins(Worker* worker);

    void break_checking();
public slots:
    void stop();
    void start();
private:
private slots:
    void check_devices();
    void toggle_stream(uint32_t user_id, Device_Item* item, bool state);
    void write_data(Device_Item* item, const QVariant& raw_data, uint32_t user_id = 0);
    void write_cache();
private:
    void write_items(DB::Plugin_Type* plugin, std::vector<Write_Cache_Item>& items);

    bool is_server_connected() const override;

    void send_stream_toggled(uint32_t user_id, Device_Item* item, bool state) override;
    void send_stream_param(Device_Item* item, const QByteArray& data) override;
    void send_stream_data(Device_Item* item, const QByteArray& data) override;

    bool b_break, first_check_;

    QTimer check_timer_, write_timer_;

    Worker* worker_;
    Scripted_Scheme* scheme_;
    std::map<DB::Plugin_Type*, std::vector<Write_Cache_Item>> write_cache_;


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
