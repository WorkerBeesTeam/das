#include "checker_test_base.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
//#include <QPluginLoader>

namespace Das {

QStringList qJsonArray_to_qStringList(const QJsonArray& arr)
{
    QStringList names;
    for (const QJsonValue& val: arr)
        names.push_back(val.toString());
    return names;
}

Checker_Test_Base::Checker_Test_Base()
{
}

bool Checker_Test_Base::is_server_connected() const
{
    return true;
}

bool Checker_Test_Base::load(const QString& file_path)
{
    std::shared_ptr<QPluginLoader> loader = std::make_shared<QPluginLoader>(file_path);
    if (!loader->load() && !loader->isLoaded())
    {
        qCritical() << "Load failed:" << loader->errorString();
        return false;
    }

    QJsonObject meta_data = loader->metaData()["MetaData"].toObject();
    QString type = meta_data["type"].toString();
    QJsonObject param = meta_data["param"].toObject();

    if (type.isEmpty() || type.length() > 128 || param.isEmpty())
    {
        loader->unload();
        qCritical() << "Bad type" << type;
        return false;
    }

    Plugin_Type* pl_type = plugin_type_mng_->get_type(type);
    if (!pl_type->id() || !pl_type->need_it || pl_type->loader)
    {
        loader->unload();
        qCritical() << "Can't find plugin type" << type;
        return false;
    }

    pl_type->checker = qobject_cast<Checker::Interface*>(loader->instance());
    if (!pl_type->checker)
    {
        loader->unload();
        qCritical() << "Can't find plugin interface" << type;
        return false;
    }

    QStringList dev_names = qJsonArray_to_qStringList(param["device"].toArray());
    if (pl_type->param_names_device() != dev_names)
    {
        qWarning() << "Plugin" << pl_type->name() << "diffrent dev_names in scheme."
                       << "\nScheme:" << pl_type->param_names_device()
                       << "\nPlugin:" << dev_names;
    }

    QStringList dev_item_names = qJsonArray_to_qStringList(param["device_item"].toArray());
    if (pl_type->param_names_device_item() != dev_item_names)
    {
        qWarning() << "Plugin" << pl_type->name() << "diffrent dev_item_names in scheme."
                       << "\nScheme:" << pl_type->param_names_device_item()
                       << "\nPlugin:" << dev_item_names;
    }

    init_checker(pl_type->checker, this);

    try {
        QSettings s;
        pl_type->checker->configure(&s);
        s.clear();
    } catch (const std::exception& e) {
        pl_type->checker = nullptr;
        loader->unload();
        qCritical() << "Can't configure plugin" << type << e.what();
        return false;
    }

    _pl_type = pl_type;
    _pl_type->loader = std::move(loader);
    return true;
}

} // namespace Das
