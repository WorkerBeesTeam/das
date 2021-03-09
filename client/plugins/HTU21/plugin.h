#ifndef DAS_HTU21PLUGIN_H
#define DAS_HTU21PLUGIN_H

#include <memory>

#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checker_interface.h>

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(HTU21Log)

class DAS_PLUGIN_SHARED_EXPORT HTU21Plugin final : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)

public:
    HTU21Plugin();
    ~HTU21Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(Device* dev, std::vector<Write_Cache_Item>& items) override;
private:
    double get_sensor_value(int fd, int sensor_id, bool& is_error_out);
    double get_temperature(int fd, bool& is_error_out);
    double get_humidity(int fd, bool &is_error_out);

    bool is_error_printed_;
};

} // namespace Das

#endif // DAS_HTU21PLUGIN_H
