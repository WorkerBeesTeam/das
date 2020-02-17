#ifndef DAS_DATABASE_DEVICE_EXTRA_PARAMS_H
#define DAS_DATABASE_DEVICE_EXTRA_PARAMS_H

#include <qobjectdefs.h>
#include <QVariant>
#include <QDataStream>

#include <Das/daslib_global.h>

namespace Das {
namespace DB {

class DAS_LIBRARY_SHARED_EXPORT Device_Extra_Params
{
public:
    Device_Extra_Params(Device_Extra_Params&& o) = default;
    Device_Extra_Params(const Device_Extra_Params& o) = default;
    Device_Extra_Params& operator =(Device_Extra_Params&& o) = default;
    Device_Extra_Params& operator =(const Device_Extra_Params& o) = default;
    Device_Extra_Params(const QVariantMap& extra = {});
    Device_Extra_Params(const QVariantList& extra_values = {});

    const QVariantMap &params() const;
    Q_INVOKABLE QVariant param(const QString& name) const;
    void set_param(const QString& name, const QVariant& value);

    QVariant extra_to_db() const;
    void set_extra_from_db(const QVariant& value);

    void set_param_name_list(const QStringList& names);
    void set_param_value_list(const QVariantList& values);
    QVariantList params_to_value_list() const;
private:
    QVariantMap extra_;
};
QDataStream &operator>>(QDataStream& ds, Device_Extra_Params& item);
QDataStream &operator<<(QDataStream& ds, const Device_Extra_Params& item);

} // namespace DB
} // namespace Das

#endif // DAS_DATABASE_DEVICE_EXTRA_PARAMS_H
