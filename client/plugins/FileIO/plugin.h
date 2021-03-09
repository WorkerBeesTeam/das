#ifndef DAS_FILEIOPLUGIN_H
#define DAS_FILEIOPLUGIN_H

#include <memory>
#include <set>

#include <QFile>
#include <QProcess>
#include <QLoggingCategory>

#include "../plugin_global.h"

#include <Das/checker_interface.h>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(FileIO_Log)

class DAS_PLUGIN_SHARED_EXPORT FileIO_Plugin final : public QObject, public Checker::Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)
public:
    FileIO_Plugin();

    void set_initializer(const QString& initializer);
    void initialize(Device* dev);
    // CheckerInterface interface
public:
    void configure(QSettings* settings) override;
    bool check(Device *dev) override;
    void stop() override;
    void write(Device* dev, std::vector<Write_Cache_Item>& items) override;
private:
    std::unique_ptr<QProcess> initializer_;
    QString prefix_;
    QFile file_;
};

} // namespace Das

#endif // DAS_FILEIOPLUGIN_H
