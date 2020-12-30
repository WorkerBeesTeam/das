
#include "worker.h"
#include "client_protocol.h"

namespace Das {
namespace Client {

Q_LOGGING_CATEGORY(NetClientLog, "net.client")

Protocol_Base::Protocol_Base(Worker* worker) :
    Helpz::Net::Protocol{},
    worker_(worker)
{
}

Worker* Protocol_Base::worker() const
{
    return worker_;
}

} // namespace Client
} // namespace Das
