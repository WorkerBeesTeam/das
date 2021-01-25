#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <QJsonArray>

#include <iostream>

#include <Das/device.h>

#include "worker.h"
#include "checker_manager.h"

namespace Das {
namespace Checker {

Q_LOGGING_CATEGORY(Log, "checker")
Q_LOGGING_CATEGORY(LogDetail, "checker.detail", QtWarningMsg)

#define MINIMAL_WRITE_INTERVAL    50

Manager::Manager(Worker *worker, QObject *parent) :
    QObject(parent),
    b_break(false), first_check_(true),
    worker_(worker),
    scheme_(worker->prj())
{
    plugin_type_mng_ = scheme_->plugin_type_mng_;
    load_plugins(worker);

    connect(scheme_, &Scheme::control_state_changed, this, &Manager::write_data, Qt::QueuedConnection);

    connect(scheme_, &Scripted_Scheme::checker_stop, this, &Manager::stop, Qt::QueuedConnection);
    connect(scheme_, &Scripted_Scheme::checker_start, this, &Manager::start, Qt::QueuedConnection);

//    connect(prj, SIGNAL(modbusRead(int,uchar,int,quint16)),
//            SLOT(read2(int,uchar,int,quint16)), Qt::BlockingQueuedConnection);
//    connect(prj, SIGNAL(modbusWrite(int,uchar,int,quint16)), SLOT(write(int,uchar,int,quint16)), Qt::QueuedConnection);

    connect(&check_timer_, &QTimer::timeout, this, &Manager::check_devices);
    //check_timer_.setInterval(interval);
    check_timer_.setSingleShot(true);

    connect(&write_timer_, &QTimer::timeout, this, &Manager::write_cache);
    write_timer_.setInterval(MINIMAL_WRITE_INTERVAL);
    write_timer_.setSingleShot(true);
    // --------------------------------------------------------------------------------

    for (Device* dev: scheme_->devices())
    {
        last_check_time_map_.insert(dev->id(), { true, 0 });
    }
    check_devices(); // Первый опрос контроллеров
    first_check_ = false;

    QMetaObject::invokeMethod(scheme_, "after_all_initialization", Qt::QueuedConnection);
}

Manager::~Manager()
{
    stop();

    for (const Plugin_Type& plugin: plugin_type_mng_->types())
        if (plugin.loader && !plugin.loader->unload())
            qCWarning(Log) << "Unload fail" << plugin.loader->fileName() << plugin.loader->errorString();
}

void Manager::break_checking()
{
    b_break = true;

    for (const Plugin_Type& plugin: plugin_type_mng_->types())
        if (plugin.loader && plugin.checker)
            plugin.checker->stop();
}

void Manager::stop()
{
    qCDebug(Log) << "Check stoped";

    if (check_timer_.isActive())
        check_timer_.stop();

    break_checking();
}

void Manager::start()
{
    qCDebug(Log) << "Start check";
    check_devices();
}

std::future<QByteArray> Manager::start_stream(uint32_t user_id, Device_Item *item, const QString &url)
{
    if (item->device())
    {
        Plugin_Type* plugin = item->device()->checker_type();
        if (plugin && plugin->checker)
            return plugin->checker->start_stream(user_id, item, url);
    }
    return {};
}

void Manager::check_devices()
{
    b_break = false;   

    qint64 next_shot, min_shot = QDateTime::currentMSecsSinceEpoch() + 60000, now_ms;
    for (Device* dev: scheme_->devices())
    {
        if (dev->check_interval() <= 0)
            continue;

        Check_Info& check_info = last_check_time_map_[dev->id()];
        now_ms = QDateTime::currentMSecsSinceEpoch();
        next_shot = check_info.time_ + dev->check_interval();

        if (next_shot <= now_ms)
        {
            if (b_break) break;

            if (dev->items().size())
            {
                if (dev->checker_type()->loader && dev->checker_type()->checker)
                {
                    if (dev->checker_type()->checker->check(dev))
                    {
                        if (!check_info.status_)
                            check_info.status_ = true;
                    }
                    else if (check_info.status_)
                    {
                        check_info.status_ = false;
                        qCDebug(Log) << "Fail check" << dev->checker_type()->name() << dev->toString();
                    }
                }
                else
                {
                    if (dev->plugin_id() == 0) // is_virtual
                    {
                        if (first_check_)
                        {
                            for (Device_Item* dev_item: dev->items())
                            {
                                if (!dev_item->is_connected())
                                {
                                    // It's only first check
                                    QMetaObject::invokeMethod(dev_item, "set_raw_value", Qt::QueuedConnection, Q_ARG(QVariant, 0));
                                    QMetaObject::invokeMethod(dev_item, "set_connection_state", Qt::QueuedConnection, Q_ARG(bool, true));
                                }
                            }
                        }
                    }
                    else
                    {
                        std::vector<Device_Item*> items;
                        for (Device_Item* dev_item: dev->items())
                            if (dev_item->is_connected())
                                items.push_back(dev_item);

                        if (!items.empty())
                        {
                            QMetaObject::invokeMethod(dev, "set_device_items_disconnect",
                                                      Q_ARG(std::vector<Device_Item*>, items));
                        }
                    }
                }
            }

            check_info.time_ = QDateTime::currentMSecsSinceEpoch();
            if (LogDetail().isDebugEnabled())
            {
                auto delta = check_info.time_ - now_ms;
                if (delta > 10)
                    qDebug(LogDetail).nospace() << delta << "ms elapsed for " << dev->name();
            }

            now_ms = check_info.time_;
            next_shot = now_ms + dev->check_interval();
        }
        min_shot = std::min(min_shot, next_shot);
    }

    if (b_break)
        return;

    now_ms = QDateTime::currentMSecsSinceEpoch();
    min_shot -= now_ms;
    if (min_shot < MINIMAL_WRITE_INTERVAL)
        min_shot = MINIMAL_WRITE_INTERVAL;
    check_timer_.start(min_shot);

    if (write_cache_.size() && !write_timer_.isActive())
        write_cache();
}

void Manager::write_data(Device_Item *item, const QVariant &raw_data, uint32_t user_id)
{
    if (!item || !item->device())
        return;

    std::vector<Write_Cache_Item>& cache = write_cache_[item->device()->checker_type()];

    auto it = std::find(cache.begin(), cache.end(), item);
    if (it == cache.end())
    {
        cache.push_back({user_id, item, raw_data});
    }
    else if (it->raw_data_ != raw_data)
    {
        it->raw_data_ = raw_data;
    }

    if (!b_break)
        write_timer_.start();
}

void Manager::write_cache()
{
    std::map<Plugin_Type*, std::vector<Write_Cache_Item>> cache(std::move(write_cache_));
    write_cache_.clear();

    while (cache.size())
    {
        write_items(cache.begin()->first, cache.begin()->second);
        cache.erase(cache.begin());
    }
}

void Manager::write_items(Plugin_Type* plugin, std::vector<Write_Cache_Item>& items)
{
    if (items.size() == 0)
    {
        return;
    }

    if (plugin && plugin->id() && plugin->checker)
    {        
        plugin->checker->write(items);
        last_check_time_map_[items.begin()->dev_item_->device_id()].time_ = 0;
    }
    else
    {
        std::map<Device_Item*, Device::Data_Item> device_items_values;
        const qint64 timestamp_msecs = DB::Log_Base_Item::current_timestamp();

        for (const Write_Cache_Item& item: items)
        {
            Device::Data_Item data_item{item.user_id_, timestamp_msecs, item.raw_data_};
            device_items_values.emplace(item.dev_item_, std::move(data_item));
        }

        if (!device_items_values.empty())
        {
            Device* dev = device_items_values.begin()->first->device();
            QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                      QArgument<std::map<Device_Item*, Device::Data_Item>>
                                      ("std::map<Device_Item*,Device::Data_Item>", device_items_values),
                                      Q_ARG(bool, true));
        }
    }
}

void Manager::load_plugins(Worker *worker)
{
    QVector<Plugin_Type> plugins_update_vect;

    QDir pluginsDir(qApp->applicationDirPath());
    pluginsDir.cd("plugins");

    std::unique_ptr<QSettings> settings;

    std::map<QString, QString> loaded_map;

    for (const QString& file_name: pluginsDir.entryList(QDir::Files))
    {
        std::shared_ptr<QPluginLoader> loader;

        try
        {
            bool need_update_param = false;
            Plugin_Type* pl_type = load_plugin(pluginsDir.absoluteFilePath(file_name), loader, need_update_param);

            init_checker(pl_type->checker, scheme_);

            if (!settings)
                settings = Worker::settings();
            pl_type->checker->configure(settings.get());

            loaded_map.emplace(pl_type->name(), file_name);

            if (need_update_param)
                plugins_update_vect.push_back(*pl_type);
        }
        catch (const std::exception& e)
        {
            if (loader->isLoaded())
                loader->unload();
            if (*e.what() != 0)
                qCCritical(Log) << "Fail to load plugin" << file_name << e.what();
        }
    }

    if (!loaded_map.empty() && Log().isDebugEnabled())
    {
        auto dbg = qDebug(Log).nospace().noquote() << "Loaded plugins:";
        for (const auto& it: loaded_map)
            dbg << "\n  - " << it.first << " (" << it.second << ')';
    }

    if (plugins_update_vect.size())
    {
        worker->update_plugin_param_names(plugins_update_vect);
    }
}

QStringList qJsonArray_to_qStringList(const QJsonArray& arr)
{
    QStringList names;
    for (const QJsonValue& val: arr)
        names.push_back(val.toString());
    return names;
}

DB::Plugin_Type *Manager::load_plugin(const QString &file_path, std::shared_ptr<QPluginLoader>& loader, bool& need_update_param)
{
    loader = std::make_shared<QPluginLoader>(file_path);
    if (!loader->load() && !loader->isLoaded())
        throw std::runtime_error(loader->errorString().toStdString());

    QJsonObject meta_data = loader->metaData()["MetaData"].toObject();
    QString type = meta_data["type"].toString();

    if (type.isEmpty() || type.length() > 128)
        throw std::runtime_error("Bad type " + type.toStdString());

    Plugin_Type* pl_type = plugin_type_mng_->get_type(type);
    if (!pl_type->id() || !pl_type->need_it || pl_type->loader)
        throw std::runtime_error("\0");

    QObject *plugin = loader->instance();
    Checker::Interface *checker_interface = qobject_cast<Checker::Interface *>(plugin);
    if (!checker_interface)
        throw std::runtime_error(loader->errorString().toStdString());

    pl_type->loader = loader;
    pl_type->checker = checker_interface;

    if (meta_data.constFind("param") != meta_data.constEnd())
    {
        QJsonObject param = meta_data["param"].toObject();

        QStringList dev_names = qJsonArray_to_qStringList(param["device"].toArray());
        if (pl_type->param_names_device() != dev_names)
        {
            qCDebug(Log) << "Plugin" << pl_type->name() << "dev_names" << pl_type->param_names_device() << dev_names;
            pl_type->set_param_names_device(dev_names);
            need_update_param = true;
        }

        QStringList dev_item_names = qJsonArray_to_qStringList(param["device_item"].toArray());
        if (pl_type->param_names_device_item() != dev_item_names)
        {
            qCDebug(Log) << "Plugin" << pl_type->name() << "dev_item_names" << pl_type->param_names_device_item() << dev_item_names;
            pl_type->set_param_names_device_item(dev_item_names);
            need_update_param = true;
        }
    }

    return pl_type;
}

bool Manager::is_server_connected() const
{
    return (bool)worker_->net_protocol();
}

} // namespace Checker
} // namespace Das
