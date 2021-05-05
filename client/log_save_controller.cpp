#include <functional>

#include <QRegularExpression>
#include <QDateTime>

#include <Das/commands.h>
#include <Das/scheme.h>
#include <Das/device.h>
#include <Das/db/device_item_value.h>
#include <plus/das/database.h>

#include "worker.h"
#include "dbus_object.h"
#include "Scripts/scripted_scheme.h"
#include "log_save_controller.h"
#include "id_timer.h"

namespace Das {

using namespace Helpz::DB;

Log_Save_Controller::Log_Save_Controller(Worker *worker) :
    prj_(worker->prj()), worker_(worker),
    _value_pack(LOG_VALUE, worker),
    _event_pack(LOG_EVENT, worker),
    _param_pack(LOG_PARAM, worker),
    _status_pack(LOG_STATUS, worker),
    _mode_pack(LOG_MODE, worker)
{
    connect(&device_item_values_timer_, &QTimer::timeout, this, &Log_Save_Controller::save_device_item_values);
    device_item_values_timer_.setSingleShot(true);

    connect(&_value_pack.timer(), &QTimer::timeout, this, &Log_Save_Controller::send_log_value_pack);
    connect(&_event_pack.timer(), &QTimer::timeout, this, &Log_Save_Controller::send_log_event_pack);
    connect(&_param_pack.timer(), &QTimer::timeout, this, &Log_Save_Controller::send_log_param_pack);
    connect(&_status_pack.timer(), &QTimer::timeout, this, &Log_Save_Controller::send_log_status_pack);
    connect(&_mode_pack.timer(), &QTimer::timeout, this, &Log_Save_Controller::send_log_mode_pack);

    const int now_msecs = QTime::currentTime().msecsSinceStartOfDay();

    const QVector<Save_Timer> save_timers = DB::Helper::get_save_timer_vect();
    for (const Save_Timer &save_timer : save_timers)
    {
        if (save_timer.interval() > 0)
        {
            ID_Timer *timer = new ID_Timer(save_timer);
            timer->setTimerType(Qt::PreciseTimer);
            connect(timer, &ID_Timer::timeout, this, &Log_Save_Controller::process_user_timer);
            timer->start(save_timer.interval() - (now_msecs % save_timer.interval()));

            timers_list_.emplace_back(timer);
        }
    }

//    connect(this, &Log_Value_Save_Timer::change, worker, &Worker::change, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::log_item_available,
            this, &Log_Save_Controller::add_log_value_item, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::add_event_message,
            this, &Log_Save_Controller::add_log_event_item, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::param_value_changed,
            this, &Log_Save_Controller::add_log_param_item, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::status_changed,
            this, &Log_Save_Controller::add_log_status_item, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::dig_mode_available,
            this, &Log_Save_Controller::add_log_mode_item, Qt::QueuedConnection);
}

Log_Save_Controller::~Log_Save_Controller()
{
    stop();
    save_device_item_values();
}

QVector<Device_Item_Value> Log_Save_Controller::get_unsaved_values() const
{
    QVector<Device_Item_Value> value_vect;
    for (auto&& it: cached_device_item_values_)
        value_vect.push_back(it.second);
    return value_vect;
}

void Log_Save_Controller::add_log_value_item(Log_Value_Item item)
{
    if (item.raw_value() == item.value())
        item.set_raw_value(QVariant());

    const bool is_big_value = item.is_big_value();

    // Process das_device_item_value table
    if (!is_big_value)
    {
        const DB::Device_Item_Value_Base& dev_item_value = static_cast<DB::Device_Item_Value_Base&>(item);
        auto it = cached_device_item_values_.find(item.item_id());
        if (it == cached_device_item_values_.end())
            cached_device_item_values_.emplace(item.item_id(), dev_item_value);
        else
            it->second = dev_item_value;
    }

    if (!device_item_values_timer_.isActive()
     || (device_item_values_timer_.remainingTime() > conf()._dev_item_value_force.count() && item.need_to_save()))
    {
        int interval = item.need_to_save() ? conf()._dev_item_value_force.count() : conf()._dev_item_value.count();
        device_item_values_timer_.start(interval);
    }
    // End das_device_item_value table

    // Process das_log_value table
    if (!_value_pack.timer().isActive()
     || (_value_pack.timer().remainingTime() > conf()._log_value_force.count() && item.need_to_save()))
    {
        int interval = item.need_to_save() ? conf()._log_value_force.count() : conf()._log_value.count();
        _value_pack.timer().start(interval);
    }
    _value_pack.data().push_back(std::move(item));

    if (is_big_value)
        send_log_value_pack();
    // End das_log_value table
}

void Log_Save_Controller::add_log_event_item(const Log_Event_Item &item)
{
    if ((item.type_id() == QtDebugMsg && item.category().startsWith("net"))
        || item.text().isEmpty() || item.category().isEmpty())
        return;

    const QString& item_text = item.text();

    Log_Event_Item* finded_event = nullptr;
    for (Log_Event_Item& event: _event_pack.data())
    {
        if (event.type_id() == item.type_id()
            && event.category() == item.category())
        {
            const QString event_text = event.text();
            if (item_text.indexOf(event_text) >= 0)
            {
                finded_event = &event;
                // TODO: regex search text in diff msg
                break;
            }
        }
    }

    if (finded_event)
    {
        QRegularExpression re("^\\([0-9]+\\) ");
        auto match = re.match(item_text);
        if (match.hasMatch())
        {
            std::cerr << "Match: " << match.capturedLength() << ' ' << match.captured(0).toStdString() << std::endl;
        }
    }
    else
        _event_pack.data().push_back(item);

    if (!_event_pack.timer().isActive() || _event_pack.data().size() < conf()._log_event_force_size)
        _event_pack.timer().start(conf()._log_event.count());
}

void Log_Save_Controller::add_log_param_item(const DIG_Param_Value &param_value)
{
    _param_pack.data().push_back(Log_Param_Item{param_value});
    _param_pack.timer().start(conf()._log_param.count());
}

void Log_Save_Controller::add_log_status_item(const DIG_Status &status)
{
    _status_pack.data().push_back(Log_Status_Item{status});

    if (!_status_pack.timer().isActive())
        _status_pack.timer().start(conf()._log_status.count());
}

void Log_Save_Controller::add_log_mode_item(const DIG_Mode &mode)
{
    _mode_pack.data().push_back(Log_Mode_Item{mode});
    _mode_pack.timer().start(conf()._log_mode.count());
}

void Log_Save_Controller::save_device_item_values()
{
    if (cached_device_item_values_.empty())
        return;

    Table table = db_table<Device_Item_Value>();

    using T = Device_Item_Value;
    QStringList field_names{
        table.field_names().at(T::COL_timestamp_msecs),
        table.field_names().at(T::COL_user_id),
        table.field_names().at(T::COL_raw_value),
        table.field_names().at(T::COL_value)
    };

    table.field_names() = std::move(field_names);

    Base& db = Base::get_thread_local_instance();

    uint32_t scheme_id = DB::Schemed_Model::default_scheme_id();
    const QString where = DB::Helper::get_default_suffix() + " AND item_id = ";

    QVariantList values{QVariant(), QVariant(), QVariant(), QVariant()};
    for (auto it: cached_device_item_values_)
    {
        const Device_Item_Value& value = it.second;
        if (value.is_big_value())
            continue;

        values.front() = value.timestamp_msecs();
        values[1] = value.user_id();
        values[2] = value.raw_value_to_db();
        values.back() = value.value_to_db();

        const QSqlQuery q = db.update(table, values, where + QString::number(value.item_id()));
        if (!q.isActive() || q.numRowsAffected() <= 0)
        {
            Table ins_table = db_table<Device_Item_Value>();
            ins_table.field_names().removeFirst(); // remove id

            QVariantList ins_values = values;
            ins_values.insert(2, value.item_id()); // after user_id
            ins_values.push_back(scheme_id);

            if (!db.insert(ins_table, ins_values))
            {
                // TODO: do something
            }
        }
    }

    cached_device_item_values_.clear();
}

void Log_Save_Controller::send_log_value_pack()
{
    if (!_value_pack.data().empty())
    {
        QMetaObject::invokeMethod(worker_->dbus(), "device_item_values_available", Qt::QueuedConnection,
                                  Q_ARG(Scheme_Info, worker_->scheme_info()),
                                  Q_ARG(QVector<Log_Value_Item>, _value_pack.data()));
    }

    _value_pack.send();
}

void Log_Save_Controller::send_log_event_pack()
{
    QMetaObject::invokeMethod(worker_->dbus(), "event_message_available", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, worker_->scheme_info()),
                              Q_ARG(QVector<Log_Event_Item>, _event_pack.data()));
    _event_pack.send();
}

void Log_Save_Controller::send_log_param_pack()
{
    if (!_param_pack.data().empty())
    {
        save_dig_param_values(_param_pack.data());

        QVector<DIG_Param_Value> param_values; // TODO: Replace type in dbus
        for (const Log_Param_Item& item: _param_pack.data())
            param_values.push_back(DIG_Param_Value{item});
        QMetaObject::invokeMethod(worker_->dbus(), "dig_param_values_changed", Qt::QueuedConnection,
                                  Q_ARG(Scheme_Info, worker_->scheme_info()),
                                  Q_ARG(QVector<DIG_Param_Value>, std::move(param_values)));
    }
    _param_pack.send();
}

void Log_Save_Controller::send_log_status_pack()
{
    QVector<DIG_Status> statuses; // TODO: Replace type in dbus
    for (const Log_Status_Item& item: _status_pack.data())
        statuses.push_back(DIG_Status{item});
    QMetaObject::invokeMethod(worker_->dbus(), "status_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, worker_->scheme_info()),
                              Q_ARG(QVector<DIG_Status>, std::move(statuses)));
    _status_pack.send();
}

void Log_Save_Controller::send_log_mode_pack()
{
    QVector<DIG_Mode> modes; // TODO: Replace type in dbus
    for (const Log_Mode_Item& item: _mode_pack.data())
    {
        DIG_Mode mode{item};
        DB::Helper::set_mode(mode);

        modes.push_back(std::move(mode));
    }
    QMetaObject::invokeMethod(worker_->dbus(), "dig_mode_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, worker_->scheme_info()),
                              Q_ARG(QVector<DIG_Mode>, std::move(modes)));
    _mode_pack.send();
}

void Log_Save_Controller::stop()
{
    device_item_values_timer_.stop();
    _value_pack.timer().stop();
    _event_pack.timer().stop();
    _param_pack.timer().stop();
    _status_pack.timer().stop();

    //timer_.stop();
    for (QTimer *timer : timers_list_)
    {
        timer->stop();
        delete timer;
    }
    timers_list_.clear();
}

void Log_Save_Controller::process_user_timer(uint32_t timer_id)
{
    Log_Value_Item pack_item{QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()};
    pack_item.set_need_to_save(true);

    Device_Item_Type_Manager* typeMng = &prj_->device_item_type_mng_;

    decltype(timer_cached_values_)::iterator val_it;

    QVariant raw_value, display_value;
    for (Device* dev: prj_->devices())
    {
        for (Device_Item* dev_item: dev->items())
        {
            if (typeMng->save_timer_id(dev_item->type_id()) != timer_id)
                continue;

            val_it = timer_cached_values_.find(dev_item->id());

            raw_value = dev_item->raw_value();
            display_value = dev_item->value();

            if (val_it == timer_cached_values_.end())
            {
                timer_cached_values_.emplace(dev_item->id(), std::make_pair(raw_value, display_value));
            }
            else if (val_it->second.first != raw_value
                     || val_it->second.first.type() != raw_value.type()
                     || val_it->second.second != display_value
                     || val_it->second.second.type() != display_value.type())
            {
                val_it->second = std::make_pair(raw_value, display_value);
            }
            else
                continue;

            pack_item.set_item_id(dev_item->id());
            pack_item.set_raw_value(raw_value);
            pack_item.set_value(display_value);
            _value_pack.data().push_back(pack_item);

            pack_item.set_timestamp_msecs(pack_item.timestamp_msecs() + 1);
        }
    }

    _value_pack.send();
}

const Log_Save_Config &Log_Save_Controller::conf() const
{
    return Log_Save_Config::instance();
}

void Log_Save_Controller::save_dig_param_values(const QVector<Log_Param_Item>& pack)
{
    if (pack.empty())
        return;

    Base& db = Base::get_thread_local_instance();
    Table table = db_table<DIG_Param_Value>();

    using T = DIG_Param_Value;
    QStringList field_names{
        table.field_names().at(T::COL_timestamp_msecs),
        table.field_names().at(T::COL_user_id),
        table.field_names().at(T::COL_value)
    };

    table.field_names() = std::move(field_names);

    uint32_t scheme_id = DB::Schemed_Model::default_scheme_id();
    const QString where = DB::Helper::get_default_suffix() + " AND group_param_id = ";
    QVariantList values{QVariant(), QVariant(), QVariant()};

    for (const Log_Param_Item& item: pack)
    {
        values.front() = item.timestamp_msecs();
        values[1] = item.user_id();
        values.back() = item.value();

        const QSqlQuery q = db.update(table, values, where + QString::number(item.group_param_id()));
        if (!q.isActive() || q.numRowsAffected() <= 0)
        {
            Table table = db_table<DIG_Param_Value>();
            table.field_names().removeFirst();

            const QVariantList ins_values{ item.timestamp_msecs(), item.user_id(), item.group_param_id(),
                                           item.value(), scheme_id };

            if (!db.insert(table, ins_values))
            {
                qCWarning(Service::Log()) << "Failed save dig param" << item.group_param_id() << item.value().size() << item.value().left(16);
                // TODO: do something
            }
        }
    }
}

} // namespace Das
