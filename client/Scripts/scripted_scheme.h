#ifndef DAS_SCRIPTED_SCHEME_H
#define DAS_SCRIPTED_SCHEME_H

#include <QScriptEngine>
#include <QtSerialBus/qmodbusdataunit.h>

#include <Helpz/simplethread.h>
#include <Helpz/db_connection_info.h>

#include <Das/scheme.h>
#include <Das/log/log_pack.h>
#include <Das/db/dig_status.h>
#include <Das/db/device_item_value.h>

#include "tools/daytimehelper.h"
#include "tools/automationhelper.h"

QT_FORWARD_DECLARE_CLASS(QScriptEngineDebugger)

class QScriptEngine;

namespace Helpz {
class ConsoleReader;
}

namespace Das {

class Worker;

class AutomationHelper;
class DayTimeHelper;

class Scripted_Scheme final : public Scheme
{
    Q_OBJECT
    Q_PROPERTY(qint64 uptime READ uptime)
public:
    enum Handler_Type {
        FUNC_UNKNOWN = 0,
        FUNC_OTHER_SCRIPTS,
        FUNC_INIT_SECTION,
        FUNC_AFTER_ALL_INITIALIZATION,
        FUNC_CHANGED_MODE,
        FUNC_CHANGED_ITEM,
        FUNC_CHANGED_SENSOR,
        FUNC_CHANGED_CONTROL,
        FUNC_CHANGED_DAY_PART,
        FUNC_CONTROL_CHANGE_CHECK,
        FUNC_AFTER_DATABASE_INIT,
        FUNC_CHECK_VALUE,
        FUNC_GROUP_STATUS,

        FUNC_COUNT
    };
    Q_ENUM(Handler_Type)

    Scripted_Scheme(Worker* worker, Helpz::ConsoleReader* consoleReader, const QString &sshHost,
                     bool allow_shell, bool only_from_folder_if_exist = false);
    ~Scripted_Scheme();

    void set_ssh_host(const QString &value);

    qint64 uptime() const;
    Section *add_section(Section&& section) override;

    QScriptValue value_from_variant(const QVariant& data) const;
signals:
    void param_value_changed(const DIG_Param_Value& param_value);
    void sct_connection_state_change(Device_Item*, bool value);
    void dig_mode_available(const DIG_Mode& mode);

    void status_changed(const DIG_Status& status);

    void checker_stop();
    void checker_start();

    void change_stream_state(uint32_t user_id, Device_Item* item, bool state);

    void add_event_message(Log_Event_Item event);
//    QVariantList modbusRead(int serverAddress, uchar registerType = QModbusDataUnit::InputRegisters,
//                                                 int startAddress = 0, quint16 unitCount = 1);
//    void modbusWrite(int server, uchar registerType, int unit, quint16 state);

    void day_time_changed(/*Section* sct*/);
public slots:
    bool stop(uint32_t user_id = 0);
    bool can_restart(bool stop = false, uint32_t user_id = 0);

    QStringList backtrace() const;
    void log(const QString& msg, uint8_t type_id, uint32_t user_id = 0, bool inform_flag = false, bool print_backtrace = false);
    void console(uint32_t user_id, const QString& cmd, bool is_function = false, const QVariantList& arguments = {});
    void reinitialization();
    void after_all_initialization();

    void ssh(uint32_t remote_port = 25589, const QString &user_name = "root", quint16 port = 22);
    QVariantMap run_command(const QString& programm, const QVariantList& args = QVariantList(), int timeout_msec = 5000) const;

    void connect_item_is_can_change(Device_Item *item, const QScriptValue& obj, const QScriptValue& func);
    void connect_item_raw_to_display(Device_Item *item, const QScriptValue& obj, const QScriptValue& func);
    void connect_item_display_to_raw(Device_Item *item, const QScriptValue& obj, const QScriptValue& func);

    QVector<DIG_Status> get_group_statuses() const;
    QVector<Device_Item_Value> get_device_item_values() const;

    void toggle_stream(uint32_t user_id, uint32_t dev_item_id, bool state);

    void write_to_item(uint32_t user_id, uint32_t item_id, const QVariant &raw_data);
    void write_to_item_file(const QString& file_name);
private slots:
    void group_initialized(Device_item_Group* group);

    bool control_change_check(Device_Item* item, const QVariant& display_value, uint32_t user_id);

    void dig_mode_changed(uint32_t user_id, uint32_t mode_id, uint32_t group_id);
    void dig_param_changed(Param *param, uint32_t user_id = 0);
    void item_changed(Device_Item* item, uint32_t user_id, const QVariant& old_raw_value);
    void handler_exception(const QScriptValue &exception);
private:
    QString handler_full_name(int handler_type) const;
    QPair<QString, QString> handler_name(int handler_type) const;
    QScriptValue get_api_obj() const;
    QScriptValue get_handler(int handler_type) const;
    QScriptValue get_handler(const QString& name, const QString& parent_name) const;
    void check_error(int handler_type, const QScriptValue &result) const;
    void check_error(const QString &str, const QScriptValue &result) const;
    void check_error(const QString &name, const QString &code) const;

    template<typename T>
    struct Type_Empty {
        QScriptValue operator() (QScriptContext*, QScriptEngine*) {
            return QScriptValue();
        }
    };
    template<typename T>
    struct Type_Default {
        QScriptValue operator() (QScriptContext*, QScriptEngine* eng) {
            return eng->newQObject(new T, QScriptEngine::ScriptOwnership);
        }
    };

    template<typename T, template<typename> class P = Type_Empty, typename... Args>
    void add_type();

    template<typename T, typename... Args>
    void add_type_n() { add_type<T, Type_Empty, Args...>(); }

    void register_types();
    void scripts_initialization(const QVector<Code_Item> &code_vect);
    QScriptValue call_function(int handler_type, const QScriptValueList& args = QScriptValueList()) const;

    QScriptEngine *script_engine_;

    DayTimeHelper day_time_;

    qint64 uptime_;

    mutable std::map<uint32_t, QScriptValue> cache_handler_;

    std::pair<uint32_t, uint32_t> last_file_item_and_user_id_;

    bool allow_shell_, only_from_folder_if_exist_;
    QString ssh_host_;
    qint64 pid = -1;

    Worker* worker_;

#ifdef QT_DEBUG
    QScriptEngineDebugger* debugger_ = nullptr;
#endif
};

} // namespace Das

#endif // DAS_SCRIPTED_SCHEME_H
