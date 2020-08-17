
#include "server_protocol.h"
#include "base_synchronizer.h"

namespace Das {

Base_Synchronizer::Base_Synchronizer(Server::Protocol_Base *protocol) :
    protocol_(protocol)
{
}

Server::Protocol_Base* Base_Synchronizer::protocol()
{
    return protocol_;
}

const Server::Protocol_Base *Base_Synchronizer::protocol() const
{
    return protocol_;
}

uint32_t Base_Synchronizer::scheme_id() const
{
    return protocol_->id();
}

QString Base_Synchronizer::title() const
{
    return protocol_->title();
}

std::shared_ptr<DB::global> Base_Synchronizer::db()
{
    return protocol_->db();
}

} // namespace Das
