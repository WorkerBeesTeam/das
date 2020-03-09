#ifndef DAS_STREAM_SERVER_H
#define DAS_STREAM_SERVER_H

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

namespace Das {

namespace Net {
class WebSocket;
}

class Stream_Server
{
public:
    Stream_Server(boost::asio::io_context& io_context, Net::WebSocket* websock, uint16_t port);
    ~Stream_Server();
private:
    void start_receive();
    void handle_receive(const boost::system::error_code &err, std::size_t size);

    boost::asio::io_context* io_context_;

    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint remote_endpoint_;

    uint8_t* recv_buffer_;
};

} // namespace Das

#endif // DAS_STREAM_SERVER_H
