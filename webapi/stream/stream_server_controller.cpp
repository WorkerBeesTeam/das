#include "stream_server_controller.h"

namespace Das {

using boost::asio::ip::udp;

Stream_Server_Controller::Stream_Server_Controller(boost::asio::io_context &io_context, uint16_t port) :
    io_context_(io_context),
    socket_(io_context, udp::endpoint(udp::v4(), port)),
    timer_(this)
{
}

} // namespace Das
