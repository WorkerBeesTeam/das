#include <QDebug>
#include <QSettings>
#include <QFileInfo>

#include <Helpz/settingshelper.h>
#include <Das/scheme.h>
#include <Das/device_item.h>
#include <Das/device.h>
#include <Das/db/device_item_type.h>

#include "plugin.h"

namespace Das {

Q_LOGGING_CATEGORY(FileIO_Log, "FileIO")

FileIO_Plugin::FileIO_Plugin() : QObject() {}

void FileIO_Plugin::set_initializer(const QString &initializer)
{
    initializer_.reset();

    if (!initializer.isEmpty())
    {
        QFileInfo info(initializer);
        if (info.exists() && info.isExecutable())
        {
            initializer_.reset(new QProcess{});
            initializer_->setProgram(initializer);
        }
    }
}

void FileIO_Plugin::initialize(Device *dev)
{
    if (initializer_)
    {
        const QVariant arg = dev->param("init_arg");
        initializer_->setArguments({arg.toString()});
        initializer_->start();
        initializer_->waitForStarted();
        initializer_->waitForFinished();
        if (initializer_->exitStatus() != QProcess::NormalExit || initializer_->exitCode() != 0)
        {
            qCWarning(FileIO_Log) << "initialize failure. Error:"
                                     << initializer_->readAllStandardError()
                                     << initializer_->readAllStandardOutput();
        }
        if (initializer_->state() != QProcess::NotRunning)
        {
            initializer_->kill();
        }
    }
}

void FileIO_Plugin::configure(QSettings *settings, Scheme *scheme)
{
    using Helpz::Param;
    auto [initializer, prefix] = Helpz::SettingsHelper(
                settings, "FileIO",
                Param{"initializer", QString()}, // /opt/das/fileio_initializer
                Param{"prefix", QString()} // /sys/class/hwmon/hwmon1/
    )();

    prefix_ = prefix;
    set_initializer(initializer);

    for (Device* dev: scheme->devices())
    {
        if (dev->checker_type()->checker == this)
        {
            initialize(dev);
        }
    }

    qCDebug(FileIO_Log) << "Configured" << initializer << prefix;
}

bool FileIO_Plugin::check(Device* dev)
{
    if (!dev)
        return false;

    const QString prefix = prefix_ + dev->param("prefix").toString();

    std::map<Device_Item*, QVariant> device_items_values;
    std::vector<Device_Item*> device_items_disconected;

    for (Device_Item* item: dev->items())
    {
        file_.setFileName(prefix + item->param("file").toString());
        if (file_.exists() && file_.open(QIODevice::ReadOnly))
        {
            const QByteArray bytes = file_.readAll();

            if (file_.error() != QFileDevice::NoError)
            {
                qCDebug(FileIO_Log) << item->toString() << "Read error:" << file_.errorString();
                device_items_disconected.push_back(item);
            }
            else
                device_items_values.emplace(item, QString::fromUtf8(bytes));

            file_.close();
        }
        else
        {
            device_items_disconected.push_back(item);
        }
    }

    if (!device_items_values.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_values", Qt::QueuedConnection,
                                  QArgument<std::map<Device_Item*, QVariant>>("std::map<Device_Item*, QVariant>", device_items_values), Q_ARG(bool, true));
    }

    if (!device_items_disconected.empty())
    {
        QMetaObject::invokeMethod(dev, "set_device_items_disconnect", Qt::QueuedConnection,
                                  Q_ARG(std::vector<Device_Item*>, device_items_disconected));

        if (device_items_values.empty())
        {
            initialize(dev);
        }
    }

    return true;
}

void FileIO_Plugin::stop() {}

void FileIO_Plugin::write(std::vector<Write_Cache_Item>& items)
{
    if (items.empty())
        return;

    for (const Write_Cache_Item& item: items)
    {
        const QString prefix = prefix_ + item.dev_item_->device()->param("prefix").toString();

        file_.setFileName(prefix + item.dev_item_->param("file").toString());
        if (file_.exists() && file_.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            file_.write(item.raw_data_.toByteArray());
            file_.waitForBytesWritten(3000);
            file_.close();

            QMetaObject::invokeMethod(item.dev_item_, "set_raw_value", Qt::QueuedConnection,
                                      Q_ARG(const QVariant&, item.raw_data_), Q_ARG(bool, false), Q_ARG(uint32_t, item.user_id_));
        }
    }
}

} // namespace Das
