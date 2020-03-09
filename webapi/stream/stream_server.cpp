#include <iostream>

#include <boost/asio/io_context.hpp>

#include <Helpz/net_defs.h>

#include "stream_server.h"

namespace Das {

using boost::asio::ip::udp;

Stream_Server::Stream_Server(boost::asio::io_context &io_context, Net::WebSocket* websock, uint16_t port) :
    socket_(io_context, udp::endpoint(udp::v4(), port)),
    recv_buffer_(new uint8_t[ HELPZ_MAX_UDP_PACKET_SIZE ])
{
    start_receive();
}

Stream_Server::~Stream_Server()
{
    delete[] recv_buffer_;
}

void Stream_Server::start_receive()
{
    auto buffer = boost::asio::buffer(recv_buffer_, HELPZ_MAX_UDP_PACKET_SIZE);

    socket_.async_receive_from(
                buffer, remote_endpoint_,
                std::bind(&Stream_Server::handle_receive, this,
                          std::placeholders::_1,   // boost::asio::placeholders::error,
                          std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void Stream_Server::handle_receive(const boost::system::error_code &err, std::size_t size)
{
    if (err)
    {
        std::cerr << "Stream_Server::handle_receive RECV ERROR " << err.category().name() << ": " << err.message() << std::endl;
        return;
    }

    std::cout << "RECV " << remote_endpoint_ << " size " << size << std::endl;
//    std::shared_ptr<Node> node = controller_->get_node(remote_endpoint);
//    if (node)
//    {
//        std::lock_guard lock(node->mutex_);
//        start_receive();

//        controller_->process_data(node, std::move(data), size);
//    }

    start_receive();
}

} // namespace Das
