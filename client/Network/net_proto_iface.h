#ifndef DAS_NET_PROTO_IFACE_H
#define DAS_NET_PROTO_IFACE_H

#include "client_protocol_latest.h"

namespace Das {

class Net_Protocol_Interface
{
public:
    virtual ~Net_Protocol_Interface() {}
    virtual std::shared_ptr<Ver::Client::Protocol> net_protocol() = 0;
};

} // namespace Das

#endif // DAS_NET_PROTO_IFACE_H
