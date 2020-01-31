
#include "worker.h"
#include "client_protocol.h"

namespace Das {
namespace Client {

Q_LOGGING_CATEGORY(NetClientLog, "net.client")

Protocol_Base::Protocol_Base(Worker* worker, const Authentication_Info &auth_info) :
    Helpz::Network::Protocol{},
    worker_(worker), auth_info_(auth_info)
{
}

Worker* Protocol_Base::worker() const
{
    return worker_;
}

const Authentication_Info &Protocol_Base::auth_info() const
{
    return auth_info_;
}

} // namespace Client
} // namespace Das
