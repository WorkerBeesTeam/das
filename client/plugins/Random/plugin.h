#ifndef DAS_RANDOMPLUGIN_H
#define DAS_RANDOMPLUGIN_H

#include <QLoggingCategory>

#include <memory>
#include <set>

#include "../plugin_global.h"
#include <Das/checkerinterface.h>

namespace Das {
namespace Random {

Q_DECLARE_LOGGING_CATEGORY(RandomLog)

class DAS_PLUGIN_SHARED_EXPORT RandomPlugin : public QObject, public Checker_Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker_Interface)
public:
    RandomPlugin();

    // CheckerInterface interface
public:
    void configure(QSettings* settings, Scheme* scheme) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(std::vector<Write_Cache_Item>& items) override;
private:
    int random(int min, int max) const;
    std::set<uint32_t> writed_list_;
    QVariant value;
};

} // namespace Random
} // namespace Das

#endif // DAS_RANDOMPLUGIN_H
