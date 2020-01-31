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
#include "log_value_save_timer.h"
#include "id_timer.h"

namespace Das {

using namespace Helpz::Database;

Log_Value_Save_Timer::Log_Value_Save_Timer(Worker *worker) :
    prj_(worker->prj()), worker_(worker)
{
    connect(&item_values_timer_, &QTimer::timeout, this, &Log_Value_Save_Timer::save_item_values);
    item_values_timer_.setSingleShot(true);

    connect(&value_pack_timer_, &QTimer::timeout, this, &Log_Value_Save_Timer::send_value_pack);
    value_pack_timer_.setSingleShot(true);

    connect(&event_pack_timer_, &QTimer::timeout, this, &Log_Value_Save_Timer::send_event_pack);
    event_pack_timer_.setSingleShot(true);

    connect(&param_values_timer_, &QTimer::timeout, this, &Log_Value_Save_Timer::send_param_pack);
    param_values_timer_.setSingleShot(true);

    const int now_msecs = QTime::currentTime().msecsSinceStartOfDay();

    const QVector<Save_Timer> save_timers = Database::Helper::get_save_timer_vect();
    for (const Save_Timer &save_timer : save_timers)
    {
        if (save_timer.interval() > 0)
        {
            ID_Timer *timer = new ID_Timer(save_timer);
            timer->setTimerType(Qt::PreciseTimer);
            connect(timer, &ID_Timer::timeout, this, &Log_Value_Save_Timer::process_items);
            timer->start(save_timer.interval() - (now_msecs % save_timer.interval()));

            timers_list_.emplace_back(timer);
        }
    }

//    connect(this, &Log_Value_Save_Timer::change, worker, &Worker::change, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::log_item_available,
            this, &Log_Value_Save_Timer::add_log_value_item, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::add_event_message,
            this, &Log_Value_Save_Timer::add_log_event_item, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::param_value_changed,
            this, &Log_Value_Save_Timer::add_param_value, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::status_added,
            this, &Log_Value_Save_Timer::status_added, Qt::QueuedConnection);
    connect(prj_, &Scripted_Scheme::status_removed,
            this, &Log_Value_Save_Timer::status_removed, Qt::QueuedConnection);
}

Log_Value_Save_Timer::~Log_Value_Save_Timer()
{
    stop();
    save_item_values();
    save_to_db(value_pack_);
    save_to_db(event_pack_);
    save_to_db(param_pack_);
}

QVector<Device_Item_Value> Log_Value_Save_Timer::get_unsaved_values() const
{
    QVector<Device_Item_Value> value_vect;
    for (auto it: waited_item_values_)
    {
        value_vect.push_back(it.second);
    }
    return value_vect;
}

void Log_Value_Save_Timer::add_log_value_item(const Log_Value_Item &item)
{
    auto waited_it = waited_item_values_.find(item.item_id());
    if (waited_it == waited_item_values_.end())
    {
        waited_item_values_.emplace(item.item_id(), Device_Item_Value{item.item_id(), item.raw_value(), item.value()});
    }
    else
    {
        waited_it->second.set_raw(item.raw_value());
        waited_it->second.set_display(item.value());
    }

    if (!item_values_timer_.isActive() || (item.need_to_save() && item_values_timer_.remainingTime() > 500))
    {
        item_values_timer_.start(item.need_to_save() ? 500 : 5000);
    }

    value_pack_.push_back(item);
    if (!value_pack_timer_.isActive() || (item.need_to_save() && value_pack_timer_.remainingTime() > 10))
    {
        value_pack_timer_.start(item.need_to_save() ? 10 : 250);
    }
}

void Log_Value_Save_Timer::add_log_event_item(const Log_Event_Item &item)
{
    if (item.type_id() == QtDebugMsg && item.category().startsWith("net"))
        return;

    const QString item_text = item.text();

    Log_Event_Item* finded_event = nullptr;
    for (Log_Event_Item& event: event_pack_)
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
        event_pack_.push_back(item);

    if (!event_pack_timer_.isActive() || event_pack_.size() < 100)
        event_pack_timer_.start(1000);
}

void Log_Value_Save_Timer::add_param_value(const DIG_Param_Value &param_value)
{
    param_pack_.push_back(param_value);
    if (!param_values_timer_.isActive())
        param_values_timer_.start(5);
}

void Log_Value_Save_Timer::status_added(uint32_t group_id, uint32_t info_id, const QStringList& args, uint32_t user_id)
{
    auto proto = worker_->net_protocol();
    if (proto)
        proto->send_status_added(group_id, info_id, args, user_id);
}

void Log_Value_Save_Timer::status_removed(uint32_t group_id, uint32_t info_id, uint32_t user_id)
{
    auto proto = worker_->net_protocol();
    if (proto)
        proto->send_status_removed(group_id, info_id, user_id);
}

void Log_Value_Save_Timer::save_item_values()
{
    if (waited_item_values_.empty())
        return;

    Table table = db_table<Device_Item_Value>();
    table.field_names().erase(table.field_names().begin(), table.field_names().begin() + 2);
    table.field_names().removeLast(); // remove scheme_id

    Base& db = Base::get_thread_local_instance();

    uint32_t scheme_id = Database::Schemed_Model::default_scheme_id();
    const QString where = Database::Helper::get_default_suffix() + " AND device_item_id = ";

    QVariantList values{QVariant(), QVariant()};
    for (auto it: waited_item_values_)
    {
        values.front() = it.second.raw_to_db();
        values.back() = it.second.display_to_db();

        const QSqlQuery q = db.update(table, values, where + QString::number(it.second.device_item_id()));
        if (!q.isActive() || q.numRowsAffected() <= 0)
        {
            Table ins_table = db_table<Device_Item_Value>();
            ins_table.field_names().removeFirst(); // remove id

            QVariantList ins_values{it.second.device_item_id()};
            ins_values += values;
            ins_values.push_back(scheme_id);

            if (!db.insert(ins_table, ins_values))
            {
                // TODO: do something
            }
        }
    }

    waited_item_values_.clear();
}

void Log_Value_Save_Timer::send_value_pack()
{
    std::shared_ptr<QVector<Log_Value_Item>> pack = std::make_shared<QVector<Log_Value_Item>>(std::move(value_pack_));
    value_pack_.clear();

    send(Ver_2_4::Cmd::LOG_PACK_VALUES, pack);

    QMetaObject::invokeMethod(worker_->dbus(), "device_item_values_available", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, worker_->scheme_info()), Q_ARG(QVector<Log_Value_Item>, *pack));
//    emit worker_->dbus()->device_item_values_available(worker_->scheme_info(), *pack);
}

void Log_Value_Save_Timer::send_event_pack()
{
    std::shared_ptr<QVector<Log_Event_Item>> pack = std::make_shared<QVector<Log_Event_Item>>(std::move(event_pack_));
    event_pack_.clear();

    send(Ver_2_4::Cmd::LOG_PACK_EVENTS, pack);

    QMetaObject::invokeMethod(worker_->dbus(), "event_message_available", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, worker_->scheme_info()), Q_ARG(QVector<Log_Event_Item>, *pack));
//    emit worker_->dbus()->event_message_available(worker_->scheme_info(), *pack);
}

void Log_Value_Save_Timer::send_param_pack()
{
    std::shared_ptr<QVector<DIG_Param_Value>> pack = std::make_shared<QVector<DIG_Param_Value>>(std::move(param_pack_));
    param_pack_.clear();

    set_dig_param_values(0, std::move(pack));
//    send(Ver_2_4::Cmd::SET_DIG_PARAM_VALUES, std::move(pack));
}

void Log_Value_Save_Timer::stop()
{
    item_values_timer_.stop();
    value_pack_timer_.stop();
    event_pack_timer_.stop();
    param_values_timer_.stop();

    //timer_.stop();
    for (QTimer *timer : timers_list_)
    {
        timer->stop();
        delete timer;
    }
    timers_list_.clear();
}

void Log_Value_Save_Timer::process_items(uint32_t timer_id)
{
    //qDebug() << "!!! process_items" << timer_id;
    //    if (timer_.interval() != (period_ * 1000))
//        timer_.setInterval(period_ * 1000);

    Log_Value_Item pack_item{QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), 0, true};

    Device_Item_Type_Manager* typeMng = &prj_->device_item_type_mng_;

    decltype(cached_values_)::iterator val_it;

    std::shared_ptr<QVector<Log_Value_Item>> pack = std::make_shared<QVector<Log_Value_Item>>();

    QVariant raw_value, display_value;
    for (Device* dev: prj_->devices())
    {
        for (Device_Item* dev_item: dev->items())
        {
            if (typeMng->save_timer_id(dev_item->type_id()) != timer_id)
                continue;

            val_it = cached_values_.find(dev_item->id());

            raw_value = dev_item->raw_value();
            display_value = dev_item->value();

            if (val_it == cached_values_.end())
            {
                cached_values_.emplace(dev_item->id(), std::make_pair(raw_value, display_value));
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
            pack->push_back(pack_item);

            pack_item.set_timestamp_msecs(pack_item.timestamp_msecs() + 1);
        }
    }

    if (!pack->empty())
        send(Ver_2_4::Cmd::LOG_PACK_VALUES, std::move(pack));
}

void Log_Value_Save_Timer::set_dig_param_values(uint32_t user_id, std::shared_ptr<QVector<DIG_Param_Value>> pack)
{
    if (pack->empty())
        return;

    QString dbg_msg = "Params changed:";
    const QString dbg_template = "\n %1: \"%2\"";

    Base& db = Base::get_thread_local_instance();
    Table table = db_table<DIG_Param_Value>();
    table.field_names().erase(table.field_names().begin(), table.field_names().begin() + 2);
    table.field_names().removeLast(); // remove scheme_id

    uint32_t scheme_id = Database::Schemed_Model::default_scheme_id();
    const QString where = Database::Helper::get_default_suffix() + " AND group_param_id = ";
    QVariantList values{QVariant()};

    for (const DIG_Param_Value& item: *pack)
    {
        dbg_msg += dbg_template.arg(item.group_param_id()).arg(item.value().left(16));
        values.front() = item.value();

        const QSqlQuery q = db.update(table, values, where + QString::number(item.group_param_id()));
        if (!q.isActive() || q.numRowsAffected() <= 0)
        {
            Table table = db_table<DIG_Param_Value>();
            table.field_names().removeFirst();

            if (!db.insert(table, {item.group_param_id(), item.value(), scheme_id}))
            {
                // TODO: do something
            }
        }
    }

    Log_Event_Item event { QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), user_id, false, QtDebugMsg, Service::Log().categoryName(), dbg_msg};
    worker_->add_event_message(std::move(event));

    QMetaObject::invokeMethod(worker_->dbus(), "dig_param_values_changed", Qt::QueuedConnection,
                              Q_ARG(Scheme_Info, worker_->scheme_info()), Q_ARG(QVector<DIG_Param_Value>, *pack));

    auto proto = worker_->net_protocol();
    if (proto)
        proto->send_dig_param_values(user_id, *pack);
}

//struct Log_PK_Increaser
//{
//    Log_PK_Increaser() : log_pk_increase_(0), last_log_pk_(0) {}
//    uint32_t log_pk_increase_;
//    qint64 last_log_pk_;
//};

//template<typename T>
//Log_PK_Increaser& get_log_increaser()
//{
//    static Log_PK_Increaser log_increaser;
//    return log_increaser;
//}

template<typename T>
void Log_Value_Save_Timer::send(uint16_t cmd, std::shared_ptr<QVector<T> > pack)
{
    if (!pack || pack->empty())
        return;

//    Log_PK_Increaser& increaser = get_log_increaser<T>();
    for (T& item: *pack)
    {
        if (item.timestamp_msecs() == 0)
        {
            qWarning() << "Attempt to save log with zero timestamp";
            item.set_timestamp_msecs(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
        }

//        if (increaser.last_log_pk_ + increaser.log_pk_increase_ >= item.timestamp_msecs())
//        {
//            increaser.log_pk_increase_ = (increaser.last_log_pk_ + increaser.log_pk_increase_) - item.timestamp_msecs();
//            increaser.last_log_pk_ = item.timestamp_msecs();
//        }

//        if (item.timestamp_msecs() == increaser.last_log_pk_)
//        {
//            item.set_timestamp_msecs(item.timestamp_msecs() + ++increaser.log_pk_increase_);
//        }
//        else
//        {
//            increaser.log_pk_increase_ = 0;
//            increaser.last_log_pk_ = item.timestamp_msecs();
//        }
    }

    auto proto = worker_->net_protocol();
    if (proto)
    {
        proto->send(cmd)
                .timeout([this, pack]()
        {
            save_to_db(*pack);
        }, std::chrono::seconds(11), std::chrono::seconds(5)) << *pack;
    }
    else
    {
        save_to_db(*pack);
    }
}

template<typename T> bool can_log_item_save(const T& /*item*/) { return true; }
template<> bool can_log_item_save<Log_Value_Item>(const Log_Value_Item& item) { return item.need_to_save(); }

template<typename T>
bool Log_Value_Save_Timer::save_to_db(const QVector<T> &pack)
{
    if (pack.empty())
        return true;

    QVariantList values, tmp_values;
    for (const T& item: pack)
    {
        if (can_log_item_save<T>(item))
        {
            tmp_values = T::to_variantlist(item);
            tmp_values.removeFirst();
            values += tmp_values;
        }
    }

    if (values.empty())
        return true;

    Table table = db_table<T>();
    table.field_names().removeFirst();

    QString sql = "INSERT INTO " + table.name() + '(' + table.field_names().join(',') + ") VALUES" +
            Base::get_q_array(table.field_names().size(), values.size() / table.field_names().size());

    Base& db = Base::get_thread_local_instance();
    return db.exec(sql, values).isActive();
}

} // namespace Das
