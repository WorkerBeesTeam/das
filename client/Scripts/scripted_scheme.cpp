#include <functional>

#include <QTimer>
#include <QElapsedTimer>

#include <QFile>
#include <QTextStream>
#include <QDirIterator>
#include <QDebug>
#include <QMetaEnum>
#include <QProcess>

#ifdef QT_DEBUG
#include <QScriptEngineDebugger>
#include <QMainWindow>
#endif

#include <Helpz/consolereader.h>
#include <Helpz/db_builder.h>

#include <Das/device.h>

#include "scripted_scheme.h"
#include "paramgroupclass.h"

#include "tools/automationhelper.h"
#include "tools/pidhelper.h"
#include "tools/severaltimeshelper.h"
#include "tools/inforegisterhelper.h"

#include "worker.h"
#include "dbus_object.h"

Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(Das::SectionPtr)
Q_DECLARE_METATYPE(Das::Type_Managers*)

namespace Das {

Q_LOGGING_CATEGORY(ScriptLog, "script")
Q_LOGGING_CATEGORY(ScriptEngineLog, "script.engine")
Q_LOGGING_CATEGORY(ScriptDetailLog, "script.detail", QtInfoMsg)

template<class T>
QScriptValue sharedPtrToScriptValue(QScriptEngine *eng, const T &obj) {
    return eng->newQObject(obj.get());
}
template<class T>
void emptyFromScriptValue(const QScriptValue &, T &) {}

QScriptValue stdstringToScriptValue(QScriptEngine *, const std::string& str)
{ return QString::fromStdString(str); }

// ------------------------------------------------------------------------------------

template<class T>
QScriptValue ptrToScriptValue(QScriptEngine *eng, const T& obj) {
    return eng->newQObject(obj);
}

struct ArgumentGetter {
    ArgumentGetter(QScriptContext* _ctx) : ctx(_ctx) {}
    QScriptContext* ctx;
    int arg_idx = 0;
    QScriptValue operator() () {
        return ctx->argument(arg_idx++);
    }
};

template<typename T, template<typename> class P, typename... Args>
void Scripted_Scheme::add_type()
{
    auto ctor = [](QScriptContext* ctx, QScriptEngine* script_engine_) -> QScriptValue {
        if ((unsigned)ctx->argumentCount() >= sizeof...(Args))
        {
            ArgumentGetter get_arg(ctx);

            auto obj = new T(qscriptvalue_cast<Args>(get_arg())...);
            return script_engine_->newQObject(obj, QScriptEngine::ScriptOwnership);
        }
        return P<T>()(ctx, script_engine_);
    };

    auto value = script_engine_->newQMetaObject(&T::staticMetaObject,
                                     script_engine_->newFunction(ctor));

    QString name = T::staticMetaObject.className();

    int four_dots = name.lastIndexOf("::");
    if (four_dots >= 0)
        name.remove(0, four_dots + 2);

    script_engine_->globalObject().setProperty(name, value);

//    qCDebug(SchemeLog) << "Register for script:" << name;
}

Scripted_Scheme::Scripted_Scheme(Worker* worker, Helpz::ConsoleReader *consoleReader, const QString &sshHost,
                                   bool allow_shell, bool only_from_folder_if_exist) :
    Scheme(),
    day_time_(this),
    uptime_(QDateTime::currentMSecsSinceEpoch()),
    allow_shell_(allow_shell),
    only_from_folder_if_exist_(only_from_folder_if_exist),
    ssh_host_(sshHost),
    worker_(worker)
{
    register_types();

    script_engine_->pushContext();
    reinitialization();

    set_ssh_host(sshHost);

    using T = Scripted_Scheme;
    connect(this, &T::mode_changed, worker, &Worker::mode_changed, Qt::QueuedConnection);
    connect(this, &T::status_changed, worker, &Worker::status_changed, Qt::QueuedConnection);
//    connect(worker, &Worker::dumpSectionsInfo, this, &T::dumpInfo, Qt::BlockingQueuedConnection);

    connect(this, &Scripted_Scheme::day_time_changed, &day_time_, &DayTimeHelper::init, Qt::QueuedConnection);
    connect(this, &T::sct_connection_state_change, worker, &Worker::connection_state_changed, Qt::QueuedConnection);

    if (consoleReader)
    {
        connect(consoleReader, &Helpz::ConsoleReader::textReceived, [this](const QString& text)
        {
            QMetaObject::invokeMethod(this, "console", Qt::QueuedConnection, Q_ARG(uint32_t, 0), Q_ARG(QString, text));
        });
    }
}

#ifdef QT_DEBUG
bool do_not_show_debugger = false;
#endif

Scripted_Scheme::~Scripted_Scheme()
{
#ifdef QT_DEBUG
    if (debugger_)
    {
        // Если закрыть окно отладчика,
        // то оно не откроется при перезагрузке кнопкой на сайте
        // Работает, но пока не понятно на сколько это удобно и нужно
        // Возможно лучше добавить диалоговое окно при закрытии отладчика
//        do_not_show_debugger = debugger_->standardWindow()->isHidden();

        debugger_->standardWindow()->close();
        debugger_->detach();
        delete debugger_;
    }
#endif
//    delete db();
    script_engine_->popContext();
}

void Scripted_Scheme::set_ssh_host(const QString &value) { if (ssh_host_ != value) ssh_host_ = value; }

qint64 Scripted_Scheme::uptime() const { return uptime_; }

void Scripted_Scheme::reinitialization()
{
    script_engine_->popContext();
    /*QScriptContext* ctx = */script_engine_->pushContext();

    std::unique_ptr<DB::Helper> db(new DB::Helper(Helpz::DB::Connection_Info::common(),
                                                              "SchemeManager_" + QString::number((quintptr)this)));
    db->fill_types(this);
    const QVector<Code_Item> code_vect = DB::Helper::get_code_item_vect();
    scripts_initialization(code_vect);

    day_time_.stop();
    qScriptDisconnect(&day_time_, SIGNAL(onDayPartChanged(Section*,bool)), QScriptValue(), QScriptValue());
    day_time_.disconnect(SIGNAL(onDayPartChanged(Section*,bool)));

    // qScriptConnect(&m_dayTime, SIGNAL(onDayPartChanged(Section*,bool)), QScriptValue(), m_func.at(FUNC_CHANGED_DAY_PART));
    connect(&day_time_, &DayTimeHelper::onDayPartChanged, [this](Section* sct, bool is_day)
    {
        call_function(FUNC_CHANGED_DAY_PART, { script_engine_->newQObject(sct), is_day });
    });

    db->init_scheme(this, true);

    if (get_handler(FUNC_CHANGED_DAY_PART).isFunction())
        day_time_.init();

    for(Section* sct: sections())
    {
        for (Device_item_Group* group: sct->groups())
        {
            connect(group, &Device_item_Group::item_changed, this, &Scripted_Scheme::item_changed);
            connect(group, &Device_item_Group::mode_changed, this, &Scripted_Scheme::dig_mode_changed);
            connect(group, &Device_item_Group::param_changed, this, &Scripted_Scheme::dig_param_changed);
            connect(group, &Device_item_Group::status_changed, this, &Scripted_Scheme::status_changed);

            connect(group, &Device_item_Group::connection_state_change, this, &Scripted_Scheme::sct_connection_state_change);

            if (get_handler(FUNC_CONTROL_CHANGE_CHECK).isFunction())
            {
                // TODO: move to Device
            }
        }
    }

    call_function(FUNC_AFTER_DATABASE_INIT);
}

void Scripted_Scheme::register_types()
{
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<Section*>("Section*");
    qRegisterMetaType<Sections*>("Sections*");
    qRegisterMetaType<Device_item_Group*>("Device_item_Group*");

    qRegisterMetaType<AutomationHelper*>("AutomationHelper*");

    script_engine_ = new QScriptEngine(this);
#ifdef QT_DEBUG
    if (qApp->thread() == thread() && !do_not_show_debugger)
    {
        debugger_ = new QScriptEngineDebugger(this);
        debugger_->attachTo(script_engine_);
        debugger_->standardWindow()->show();
    }
#endif

    connect(script_engine_, &QScriptEngine::signalHandlerException,
            this, &Scripted_Scheme::handler_exception);

    add_type<QTimer>();
    add_type<QProcess>();

    add_type<AutomationHelper, Type_Default, uint>();

    add_type_n<AutomationHelperItem, Device_item_Group*>();
    add_type_n<SeveralTimesHelper, Device_item_Group*>();
    add_type_n<PIDHelper, Device_item_Group*, uint>();
    add_type_n<InfoRegisterHelper, Device_item_Group*, uint, uint>();

    add_type_n<Section, uint32_t, QString, uint32_t, uint32_t>();
    add_type_n<Device_item_Group, uint32_t, QString, uint32_t, uint32_t, uint32_t>();

    qRegisterMetaType<Sections>("Sections");
    qScriptRegisterSequenceMetaType<Sections>(script_engine_);

    qRegisterMetaType<Devices>("Devices");
    qScriptRegisterSequenceMetaType<Devices>(script_engine_);

    qRegisterMetaType<Device_Item*>("Device_Item*");
    qRegisterMetaType<Device_Items>("Device_Items");
    qScriptRegisterSequenceMetaType<Device_Items>(script_engine_);

    qRegisterMetaType<QVector<Device_Item*>>("QVector<Device_Item*>"); // REVIEW а разве не тоже самое, что и  qRegisterMetaType<Device_Items>("Device_Items");
    qScriptRegisterSequenceMetaType<QVector<Device_Item*>>(script_engine_);

    qRegisterMetaType<QVector<Device_item_Group*>>("QVector<Device_item_Group*>");
    qScriptRegisterSequenceMetaType<QVector<Device_item_Group*>>(script_engine_);

    qRegisterMetaType<AutomationHelperItem*>("AutomationHelperItem*");

//    qScriptRegisterMetaType<SectionPtr>(eng, sharedPtrToScriptValue<SectionPtr>, emptyFromScriptValue<SectionPtr>);
    qScriptRegisterMetaType<std::string>(script_engine_, stdstringToScriptValue, emptyFromScriptValue<std::string>);
//    qScriptRegisterMetaType<Device_Item*>(eng, )

//    qScriptRegisterMetaType<Device_Item::ValueType>(eng, itemValueToScriptValue, itemValueFromScriptValue);
    //    qScriptRegisterSequenceMetaType<Device_Item::ValueList>(eng);

    //    qScriptRegisterSequenceMetaType<Device_Item::ValueList>(eng);

    auto paramClass = new ParamGroupClass(script_engine_);
    script_engine_->globalObject().setProperty("Params", paramClass->constructor());

//    eng->globalObject().setProperty("ParamElem", paramClass->constructor());
}

template<typename T>
void init_types(QScriptValue& parent, const QString& prop_name, DB::Named_Type_Manager<T>& type_mng)
{
    QScriptValue types_obj = parent.property(prop_name);
    QString name;
    for (const T& type: type_mng.types())
    {
        name = type.name();
        if (!name.isEmpty())
        {
            types_obj.setProperty(name, type.id());
        }
    }
}

void Scripted_Scheme::scripts_initialization(const QVector<Code_Item>& code_vect)
{
    auto read_script_file = [this](const QString& file_path)
    {
        QFile script_file(file_path);
        if (script_file.exists())
        {
            if (script_file.open(QIODevice::ReadOnly | QFile::Text))
            {
                QTextStream stream(&script_file);
                check_error(script_file.fileName(), stream.readAll());
                script_file.close();
            }
            else
                qCWarning(SchemeLog) << "Can't load file:" << file_path;
        }
        else
            qCWarning(SchemeLog) << "Attempt to load a nonexistent file:" << file_path;
    };

    read_script_file(":/Scripts/js/api.js");

    QScriptValue api = get_api_obj();
    api.setProperty("mng", script_engine_->newQObject(this));

    QScriptValue statuses = api.property("status");
    QScriptValue common_status = script_engine_->newObject();
    api.setProperty("common_status", common_status);

    // Многие статусы могут пренадлежать одному типу группы.
    QString status_name;
    QScriptValue status_group_property;
    for (DIG_Status_Type& type: *status_mng_.get_types())
    {
        if (type.group_type_id)
        {
            status_name = group_type_mng_.name(type.group_type_id);
            status_group_property = statuses.property(status_name);
            if (!status_group_property.isObject())
            {
                status_group_property = script_engine_->newObject();
                statuses.setProperty(status_name, status_group_property);
            }
            status_group_property.setProperty(type.name(), type.id());
        }
        else
        {
            common_status.setProperty(type.name(), type.id());
        }
    }

    QScriptValue types = api.property("type");
    init_types(types, "item", device_item_type_mng_);
    init_types(types, "group", group_type_mng_);
    init_types(types, "mode", dig_mode_type_mng_);
    init_types(types, "param", param_mng_);

    // Begin load user scripts

    std::function<void(const QString&)> load_scripts_from_dir = [&load_scripts_from_dir, &read_script_file](const QString& dir_path)
    {
        qCDebug(ScriptDetailLog) << "load_scripts_from_dir begin" << dir_path;
        QDir scripts(dir_path);
        QFileInfoList js_list = scripts.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        for (const QFileInfo& file_info: js_list)
        {
            if (file_info.isDir())
            {
                load_scripts_from_dir(file_info.filePath());
            }
            else if (file_info.suffix() == "js")
            {
                qCDebug(ScriptDetailLog) << "load_scripts_from_dir load" << file_info.filePath();
                read_script_file(file_info.filePath());
            }
        }
    };

    QString script_dir_path = qApp->applicationDirPath() + "/scripts/";

    if (!only_from_folder_if_exist_ || !QDir(script_dir_path).exists())
    {
        for (const Code_Item& code_item: code_vect)
            check_error(code_item.name(), code_item.text);
    }

    load_scripts_from_dir(script_dir_path);

    // Begin search for handlers in loaded scripts

    QScriptValue group_handler_obj = get_handler("changed", "group"), group_handler;
    for (const DIG_Type& type: group_type_mng_.types())
    {
        if (type.name().isEmpty())
            qCDebug(ScriptDetailLog) << "Group type" << type.id() << type.name() << "havent latin name";
        else
        {
            group_handler = group_handler_obj.property(type.name());
            if (group_handler.isFunction())
                cache_handler_.emplace(FUNC_COUNT + type.id(), group_handler);
            else
                qCDebug(ScriptDetailLog) << "Group type" << type.id() << type.name() << "havent 'changed' function" << group_handler_obj.toString();
        }
    }

    // Begin load script checkers

    QScriptValue checker_list = api.property("checker");
    if (checker_list.isArray())
    {
        int length = checker_list.property("length").toInt32();
        for (int i = 0; i < length; ++i)
        {
            // TODO: Load Script checkers
        }
        qCDebug(ScriptDetailLog) << "Load" << length << "script checkers";
    }
}

Section *Scripted_Scheme::add_section(Section&& section)
{
    auto sct = Scheme::add_section(std::move(section));
    connect(sct, &Section::group_initialized, this, &Scripted_Scheme::group_initialized);

    call_function(FUNC_INIT_SECTION, { script_engine_->newQObject(sct) });
    return sct;
}

QScriptValue Scripted_Scheme::value_from_variant(const QVariant &data) const
{
    switch (data.type()) {
    case QVariant::Bool:                return data.toBool();
    case QVariant::String:              return data.toString();
    case QVariant::Int:                 return data.toInt();
    case QVariant::UInt:                return data.toUInt();
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:              return data.toDouble();
    case QVariant::Date:
    case QVariant::Time:
    case QVariant::DateTime:            return script_engine_->newDate(data.toDateTime());
    case QVariant::RegExp:
    case QVariant::RegularExpression:   return script_engine_->newRegExp(data.toRegExp());
    default:
        return  script_engine_->newVariant(data);
    }
}

bool Scripted_Scheme::stop(uint32_t user_id)
{
    return can_restart(true, user_id);
}

bool Scripted_Scheme::can_restart(bool stop, uint32_t user_id)
{
    QScriptValue can_restart = get_api_obj().property("handlers").property("can_restart");
    if (can_restart.isFunction())
    {
        QScriptValue ret = can_restart.call(QScriptValue(), QScriptValueList{stop, user_id});
        if (ret.isBool() && !ret.toBool())
        {
            return false;
        }
    }

    return true;
}

QStringList Scripted_Scheme::backtrace() const
{
    return script_engine_->currentContext()->backtrace();
}

void Scripted_Scheme::log(const QString &msg, uint8_t type_id, uint32_t user_id, bool inform_flag, bool print_backtrace)
{
    Log_Event_Item event{ QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), user_id, inform_flag, type_id, ScriptLog().categoryName(), msg };
    if (print_backtrace)
        event.set_text(event.text() + "\n\n" + script_engine_->currentContext()->backtrace().join('\n'));
    std::cerr << "[script] " << event.text().toStdString() << std::endl;
    add_event_message(std::move(event));
}

void Scripted_Scheme::console(uint32_t user_id, const QString &cmd, bool is_function, const QVariantList& arguments)
{
    if (is_function)
    {
        QScriptValueList arg_list;
        for (const QVariant& arg: arguments)
            arg_list.push_back(script_engine_->newVariant(arg));

        QScriptValue func = script_engine_->currentContext()->activationObject().property(cmd);
        QScriptValue ret = func.call(QScriptValue(), arg_list);
        check_error( "exec", ret);
        return;
    }

    QString script = cmd.trimmed();
    if (script.isEmpty() || !script_engine_->canEvaluate(script))
        return;

    QScriptValue res = script_engine_->evaluate(script, "CONSOLE");

    bool is_error = res.isError();

    if (!res.isUndefined())
    {
        if (!res.isError() && (res.isObject() || res.isArray()))
        {
            QScriptValue check_func = script_engine_->evaluate(
                "(function(key, value) {"
                "  if ((typeof value === 'object' || typeof value === 'function') && "
                "      value !== null && !Array.isArray(value) && value.toString() !== '[object Object]') {"
                "    return value.toString();"
                "  }"
                "  return value;"
                "})");
            res = script_engine_->evaluate("JSON.stringify").call(QScriptValue(), {res, check_func, ' '});
        }

        if (res.isError())
        {
            res = res.property("message");
            is_error = true;
        }
    }
    Log_Event_Item event{ QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), user_id, false, is_error ? QtCriticalMsg : QtInfoMsg,
                ScriptLog().categoryName(), "CONSOLE [" + script + "] >" + res.toString() };
    std::cerr << event.text().toStdString() << std::endl;
    add_event_message(std::move(event));
}

QScriptValue Scripted_Scheme::call_function(int handler_type, const QScriptValueList& args) const
{
    QScriptValue handler = get_handler(handler_type);
    if (handler.isFunction())
    {
        QScriptValue ret = handler.call(QScriptValue(), args);
        check_error( handler_type, ret);
        return ret;
    }

    return QScriptValue();
}

void Scripted_Scheme::dig_mode_changed(uint32_t user_id, uint32_t mode_id, uint32_t group_id)
{
    auto group = static_cast<Device_item_Group*>(sender());
    if (!group)
        return;

    emit dig_mode_available(group->mode_data());

    QScriptValue groupObj = script_engine_->newQObject(group);

    call_function(FUNC_CHANGED_MODE, { groupObj, mode_id, user_id });
    call_function(FUNC_COUNT + group->type_id(), { groupObj, QScriptValue(), user_id });
}

void Scripted_Scheme::dig_param_changed(Param* param, uint32_t user_id)
{
    auto group = static_cast<Device_item_Group*>(sender());
    if (!group)
        return;

    DIG_Param_Value dig_param_value{DB::Log_Base_Item::current_timestamp(), user_id, param->id(), param->value().toString()};
    emit param_value_changed(dig_param_value);

    call_function(FUNC_COUNT + group->type_id(), { script_engine_->newQObject(group), QScriptValue(), user_id });
}

QElapsedTimer t;
void Scripted_Scheme::item_changed(Device_Item *item, uint32_t user_id, const QVariant& old_raw_value)
{
    if (!item)
        return;

    auto group = static_cast<Device_item_Group*>(sender());
    if (!group)
        return;

    uint8_t save_algorithm = device_item_type_mng_.save_algorithm(item->type_id());
    if (save_algorithm == Device_Item_Type::SA_INVALID)
    {
        qWarning(SchemeLog).nospace() << user_id << "|Неправильный параметр сохранения: " << item->toString();
    }
    bool immediately = save_algorithm == Device_Item_Type::SA_IMMEDIATELY;
    Log_Value_Item log_value_item = reinterpret_cast<const Log_Value_Item&>(item->data());
    log_value_item.set_need_to_save(immediately);
    emit log_item_available(log_value_item);

    QScriptValue groupObj = script_engine_->newQObject(group);
    QScriptValue itemObj = script_engine_->newQObject(item);

    QScriptValueList args { groupObj, itemObj, user_id, value_from_variant(old_raw_value) };

    t.restart();
    call_function(FUNC_CHANGED_ITEM, args);

    if (item->is_control())
        call_function(FUNC_CHANGED_CONTROL, args);
    else
        call_function(FUNC_CHANGED_SENSOR, args);

    call_function(FUNC_COUNT + group->type_id(), args);

//    eng->collectGarbage();

    if (t.elapsed() > 500)
        qCWarning(ScriptEngineLog) << "item_changed timeout" << t.elapsed();

    t.invalidate();
}

void Scripted_Scheme::after_all_initialization()
{
    call_function(FUNC_AFTER_ALL_INITIALIZATION);
}

void Scripted_Scheme::ssh(uint32_t remote_port, const QString& user_name, quint16 port)
{
    if (pid > 0)
        QProcess::startDetached("kill", { QString::number(pid) });
    // ssh -o StrictHostKeyChecking=no -N -R 0.0.0.0:12345:localhost:22 -p 15666 root@deviceaccess.ru
    QStringList args{"-o", "StrictHostKeyChecking=no", "-N", "-R", QString("0.0.0.0:%1:localhost:22").arg(remote_port), "-p", QString::number(port), user_name + '@' + ssh_host_ };
    QProcess::startDetached("ssh", args, QString(), &pid);

    qCInfo(SchemeLog) << "SSH Client started" << pid;
}

QVariantMap Scripted_Scheme::run_command(const QString &programm, const QVariantList &args, int timeout_msec) const
{
    QString error;
    int exit_code = 0;
    QVariantMap result;
    if (allow_shell_)
    {
        QStringList args_list;
        for (const QVariant& arg: args)
            args_list.push_back(arg.toString());

        QProcess proc;
        proc.start(programm, args_list);
        if (proc.waitForStarted(timeout_msec) && proc.waitForFinished(timeout_msec) && proc.bytesAvailable())
            result["output"] = QString::fromUtf8(proc.readAllStandardOutput());

        exit_code = proc.exitCode();
        error = QString::fromUtf8(proc.readAllStandardError());
    }

    result["code"] = exit_code;
    result["error"] = error;
    return result;
}

void Scripted_Scheme::connect_item_is_can_change(Device_Item *item, const QScriptValue& obj, const QScriptValue& func)
{
    if (item && func.isFunction())
    {
        connect(item, &Device_Item::is_can_change, [this, obj, func](const QVariant& display_value, uint32_t user_id) -> bool
        {
            QScriptValue f = func;
            QScriptValue res = f.call(obj, QScriptValueList{ value_from_variant(display_value), user_id });
            return res.toBool();
        });
    }
}

void Scripted_Scheme::connect_item_raw_to_display(Device_Item *item, const QScriptValue &obj, const QScriptValue &func)
{
    if (item && func.isFunction())
    {
        connect(item, &Device_Item::raw_to_display, [this, obj, func](const QVariant& data) -> QVariant
        {
            QScriptValue f = func;
            QScriptValue res = f.call(obj, QScriptValueList{ value_from_variant(data) });
            return res.toVariant();
        });
    }
}

void Scripted_Scheme::connect_item_display_to_raw(Device_Item *item, const QScriptValue &obj, const QScriptValue &func)
{
    if (item && func.isFunction())
    {
        connect(item, &Device_Item::display_to_raw, [this, obj, func](const QVariant& data) -> QVariant
        {
            QScriptValue f = func;
            QScriptValue res = f.call(obj, QScriptValueList{ value_from_variant(data) });
            return res.toVariant();
        });
    }
}

QVector<DIG_Status> Scripted_Scheme::get_group_statuses() const
{
    QVector<DIG_Status> status_vect;

    for (const Section* sct: sections())
    {
        for (const Device_item_Group* group: sct->groups())
        {
            for (const DIG_Status& status: group->get_statuses())
            {
                status_vect.push_back(status);
            }
        }
    }

    return status_vect;
}

QVector<Device_Item_Value> Scripted_Scheme::get_device_item_values() const
{
    QVector<Device_Item_Value> values;
    for (const Device* dev: devices())
    {
        for (const Device_Item* item: dev->items())
        {
            values.push_back(item->data());
        }
    }
    return values;
}

void Scripted_Scheme::toggle_stream(uint32_t user_id, uint32_t dev_item_id, bool state)
{
    Device_Item* item = item_by_id(dev_item_id);
    if (item)
        emit change_stream_state(user_id, item, state);
}

void Scripted_Scheme::write_to_item(uint32_t user_id, uint32_t item_id, const QVariant& raw_data)
{
    if (Device_Item* item = item_by_id(item_id))
    {
        if (item->register_type() == Device_Item_Type::RT_FILE)
        {
            last_file_item_and_user_id_ = std::make_pair(item_id, user_id);
        }

        item->write(raw_data, 0, user_id);
    }
    else
    {
        qCWarning(SchemeLog).nospace() << user_id << "| item for write not found. item_id: " << item_id;
    }
}

void Scripted_Scheme::write_to_item_file(const QString& file_name)
{
    if (Device_Item* item = item_by_id(last_file_item_and_user_id_.first))
    {
        qCDebug(SchemeLog) << "write_to_item_file" << file_name;
        item->write(file_name, 0, last_file_item_and_user_id_.second);
    }
    else
    {
        qCWarning(SchemeLog).nospace() << last_file_item_and_user_id_.second << "| item for write file not found. item_id: " << last_file_item_and_user_id_.first;
    }
}

void Scripted_Scheme::group_initialized(Device_item_Group* group)
{
    QString group_type_name = group_type_mng_.name(group->type_id()),
            func_name = "api.handlers.group.initialized." + group_type_name;
    QScriptValue group_handler_obj = get_handler("initialized", "group");
    QScriptValue group_handler_init = group_handler_obj.property(group_type_name);

    if (group_handler_init.isFunction())
    {
        auto ret = group_handler_init.call(QScriptValue(), QScriptValueList{ script_engine_->newQObject(group) });
        check_error( func_name, ret);
    }
    else
        qCDebug(ScriptDetailLog) << "Group type" << group->type_id() << group_type_name << "havent init function" << func_name;
}

bool Scripted_Scheme::control_change_check(Device_Item *item, const QVariant &display_value, uint32_t user_id)
{
    auto ret = call_function(FUNC_CONTROL_CHANGE_CHECK, { script_engine_->newQObject(sender()), script_engine_->newQObject(item), value_from_variant(display_value), user_id });
    return ret.isBool() && ret.toBool();
}

void Scripted_Scheme::handler_exception(const QScriptValue &exception)
{
    if (script_engine_->hasUncaughtException())
        qCCritical(ScriptEngineLog) << "Exception:" << exception.toString() << script_engine_->uncaughtExceptionBacktrace()
                                    << '\n' << backtrace().join('\n');
}

QString Scripted_Scheme::handler_full_name(int handler_type) const
{
    QPair<QString,QString> names;
    if (handler_type > FUNC_COUNT)
    {
        handler_type -= FUNC_COUNT;
        names.first = "group";
        names.second = "changed." + group_type_mng_.name(handler_type);
    }
    else
        names = handler_name(handler_type);
    QString full_name = "api.handlers.";
    if (!names.first.isEmpty())
        full_name += names.first + '.';
    full_name += names.second;
    return full_name;
}

QPair<QString,QString> Scripted_Scheme::handler_name(int handler_type) const
{
    static const char* changed = "changed";
    static const char* section = "section";
    static const char* database = "database";
    static const char* initialized = "initialized";

    switch (static_cast<Handler_Type>(handler_type))
    {
    case FUNC_INIT_SECTION:                 return {section, initialized};
    case FUNC_AFTER_ALL_INITIALIZATION:     return {{}, initialized};
    case FUNC_CHANGED_MODE:                 return {changed, "mode"};
    case FUNC_CHANGED_ITEM:                 return {changed, "item"};
    case FUNC_CHANGED_SENSOR:               return {changed, "sensor"};
    case FUNC_CHANGED_CONTROL:              return {changed, "control"};
    case FUNC_CHANGED_DAY_PART:             return {changed, "day_part"};
    case FUNC_CONTROL_CHANGE_CHECK:         return {{}, "control_change_check"};
    case FUNC_AFTER_DATABASE_INIT:          return {database, initialized};
    case FUNC_CHECK_VALUE:                  return {{}, "check_value"};
    case FUNC_GROUP_STATUS:                 return {{}, "group_status"};
    default:
        break;
    }
    return {};
}

QScriptValue Scripted_Scheme::get_api_obj() const
{
    return script_engine_->currentContext()->activationObject().property("api");
}

QScriptValue Scripted_Scheme::get_handler(int handler_type) const
{
    auto it = cache_handler_.find(handler_type);
    if (it != cache_handler_.cend())
    {
        return it->second;
    }
    auto name_pair = handler_name(handler_type);
    QScriptValue handler = get_handler(name_pair.second, name_pair.first);
    if (handler.isFunction())
        cache_handler_.emplace(handler_type, handler);
    else
        qCDebug(ScriptDetailLog) << "Can not find:" << handler_full_name(handler_type);
    return handler;
}

QScriptValue Scripted_Scheme::get_handler(const QString& name, const QString& parent_name) const
{
    QScriptValue obj = get_api_obj().property("handlers");
    if (!parent_name.isEmpty())
    {
        obj = obj.property(parent_name);
        if (!obj.isValid())
        {
            qCDebug(ScriptDetailLog) << "Can not find property" << parent_name << "in api.handlers";
            return obj;
        }
    }
    return obj.property(name);
}

void Scripted_Scheme::check_error(int handler_type, const QScriptValue& result) const
{
    if (result.isError())
    {
        check_error(handler_full_name(handler_type), result);
        cache_handler_.erase(handler_type);
    }
}

void Scripted_Scheme::check_error(const QString& str, const QScriptValue& result) const
{
    if (result.isError())
    {
        qCCritical(ScriptEngineLog).noquote().nospace()
                << str << ' ' << result.property("fileName").toString()
                << '(' << result.property("lineNumber").toInt32() << "): "
                << result.toString() << '\n' << backtrace().join('\n');
    }
}

void Scripted_Scheme::check_error(const QString &name, const QString &code) const
{
    if (code.isEmpty())
        qCWarning(SchemeLog) << "Attempt to evaluate a empty file:" << name;
    else
        check_error(name, script_engine_->evaluate(code, name));
}

} // namespace Das
