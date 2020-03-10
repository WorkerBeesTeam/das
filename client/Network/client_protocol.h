#ifndef DAS_NET_PROTOCOL_H
#define DAS_NET_PROTOCOL_H

#include <QLoggingCategory>

#include <Helpz/net_protocol.h>

#include <plus/das/authentication_info.h>

namespace Das {

class Worker;

namespace Client {

Q_DECLARE_LOGGING_CATEGORY(NetClientLog)

class Protocol_Base : public Helpz::Net::Protocol
{
public:
    Protocol_Base(Worker* worker, const Authentication_Info &auth_info);
    virtual ~Protocol_Base() = default;

    Worker* worker() const;

    const Authentication_Info& auth_info() const;

private:
    Worker *worker_;
    Authentication_Info auth_info_;
};

} // namespace Client
} // namespace Das

#endif // DAS_NET_PROTOCOL_H
