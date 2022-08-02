#ifndef DAS_GATT_CHARACTERISTIC_RECEIVER_H
#define DAS_GATT_CHARACTERISTIC_RECEIVER_H

#include <Das/device_item.h>

#include <QTimer>

namespace Das {
namespace Gatt {

class Characteristic_Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Characteristic_Receiver(Device_Item* item, QString&& characteristic, QObject *parent = nullptr);
    ~Characteristic_Receiver();

    Device_Item* item() const;
    const QString& uuid() const;
    const QString& path() const;

    void set_path(const QString& path);
    void stop();

signals:
    void timeout(Device_Item* item);

public slots:
    void dev_props_changed(const QString& /*iface*/, const QVariantMap& changed, const QStringList& /*invalidated*/);

private slots:
    void emit_timeout();

private:
    Device_Item* _item;
    QString _uuid;
    QString _path;

    QTimer _timer;
};

} // namespace Gatt
} // namespace Das

#endif // DAS_GATT_CHARACTERISTIC_RECEIVER_H
