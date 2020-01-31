#ifndef DAS_PLUGINTYPEMANAGER_H
#define DAS_PLUGINTYPEMANAGER_H

#include <QPluginLoader>
#include <memory>

#include <Das/checkerinterface.h>
#include <Helpz/db_meta.h>
#include "base_type.h"

namespace Das {
namespace Database {

class DAS_LIBRARY_SHARED_EXPORT Plugin_Type : public Base_Type
{
    HELPZ_DB_META(Plugin_Type, "plugin_type", "pt", 5, DB_A(id), DB_A(name),
                  DB_AT(param_names_device), DB_AT(param_names_device_item), DB_A(scheme_id))
public:
    Plugin_Type(Plugin_Type&& other) = default;
    Plugin_Type(const Plugin_Type& other) = default;
    Plugin_Type& operator=(Plugin_Type&& other) = default;
    Plugin_Type& operator=(const Plugin_Type& other) = default;
    Plugin_Type(uint32_t id = 0, const QString& name = QString(),
                const QStringList& param_names_device = QStringList(),
                const QStringList& param_names_device_item = QStringList());

    bool need_it = false;
    std::shared_ptr<QPluginLoader> loader;
    Checker_Interface* checker = nullptr;

    const QStringList& param_names_device() const;
    void set_param_names_device(const QStringList& param_names_device);

    QVariant param_names_device_to_db() const;
    void set_param_names_device_from_db(const QVariant& data);

    const QStringList& param_names_device_item() const;
    void set_param_names_device_item(const QStringList& param_names_device_item);

    QVariant param_names_device_item_to_db() const;
    void set_param_names_device_item_from_db(const QVariant& data);

private:
    QStringList param_names_device_, param_names_device_item_;

    friend QDataStream& operator>>(QDataStream& ds, Plugin_Type& item);
};

QDataStream& operator<<(QDataStream& ds, const Plugin_Type& item);
QDataStream& operator>>(QDataStream& ds, Plugin_Type& item);

struct DAS_LIBRARY_SHARED_EXPORT Plugin_Type_Manager : public Base_Type_Manager<Plugin_Type> {};

} // namespace Database

using Plugin_Type = Database::Plugin_Type;
using Plugin_Type_Manager = Database::Plugin_Type_Manager;

} // namespace Das

#endif // DAS_PLUGINTYPEMANAGER_H
