#ifndef DAS_MODBUS_THREAD_H
#define DAS_MODBUS_THREAD_H

#include <QThread>

#include "modbus_controller.h"

namespace Das {
namespace Modbus {

class Thread final : public QThread
{
    Q_OBJECT
public:
    explicit Thread(const Config& config, QObject *parent = nullptr);
    ~Thread();

    Controller* controller();

private:
    Controller* _ctrl;
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUS_THREAD_H
