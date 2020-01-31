#ifndef DAS_HTU21PLUGIN_H
#define DAS_HTU21PLUGIN_H

#include <memory>

#include <QLoggingCategory>

#include "../plugin_global.h"
#include <Das/checkerinterface.h>

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(DS18B20Log)

class One_Wire;
class DAS_PLUGIN_SHARED_EXPORT DS18B20_Plugin : public QObject, public Checker_Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker_Interface)

public:
    DS18B20_Plugin();
    ~DS18B20_Plugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings, Scheme*) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
private:
    void search_rom();
    double get_temperature(uint32_t num, bool& is_ok);

    bool is_error_printed_;

    std::unique_ptr<uint64_t[]> rom_array_;
    uint32_t rom_count_;

    One_Wire* one_wire_;
};

} // namespace Das

#endif // DAS_HTU21PLUGIN_H
