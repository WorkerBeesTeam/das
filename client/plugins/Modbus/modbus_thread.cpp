#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>

#include "modbus_log.h"
#include "modbus_thread.h"

namespace Das {
namespace Modbus {

Thread::Thread(const Config& config, QObject *parent)
    : QThread(parent)
{
    _ctrl = new Controller(config);
    _ctrl->moveToThread(this);
    connect(this, &Thread::started, _ctrl, &Controller::init, Qt::DirectConnection);
    connect(this, &Thread::finished, _ctrl, &Controller::deinit, Qt::DirectConnection);
    start();
}

Thread::~Thread()
{
    if (!isFinished())
    {
        quit();
        wait();
    }

    delete _ctrl;
}

Controller *Thread::controller()
{
    return _ctrl;
}

} // namespace Modbus
} // namespace Das
