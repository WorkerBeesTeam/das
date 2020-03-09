#ifndef DAS_STREAM_SERVER_H
#define DAS_STREAM_SERVER_H

#include <map>
#include <set>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

#include "stream_server_controller.h"
#include "stream_node.h"

namespace Das {

namespace Net {
class WebSocket;
}

class Stream_Server : public Stream_Server_Controller
{
public:
    Stream_Server(boost::asio::io_context& io_context, Net::WebSocket* websock, uint16_t port);
    ~Stream_Server();

    void add_stream(qint64 param, uint32_t scheme_id, uint32_t dev_item_id);
    void remove_stream(uint32_t scheme_id, uint32_t dev_item_id);
private:

    struct Stream_Info : public Stream_Node::Stream_Info {
        std::set<boost::asio::ip::udp::endpoint> endpoints_;
    };

    void erase_info(const Stream_Info& info);
    void start_receive();
    void handle_receive(const boost::system::error_code &err, std::size_t size);

    void on_protocol_timeout(boost::asio::ip::udp::endpoint endpoint, void *data) override;
    void send_frame(boost::asio::ip::udp::endpoint remote_endpoint, qint64 param, uint32_t dev_item_id, const QByteArray& buffer) override;
    void remove(boost::asio::ip::udp::endpoint remote_endpoint) override;

    void cleaning(const boost::system::error_code &err);

    boost::asio::ip::udp::endpoint remote_endpoint_;

    std::chrono::seconds cleaning_timeout_;
    boost::asio::steady_timer cleaning_timer_;

    uint8_t* recv_buffer_;

    std::map<boost::asio::ip::udp::endpoint, std::shared_ptr<Stream_Node>> clients_;

    std::map<qint64, Stream_Info> stream_map_;

    Net::WebSocket* websock_;
};

} // namespace Das

#endif // DAS_STREAM_SERVER_H
