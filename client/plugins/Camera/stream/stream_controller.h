#ifndef DAS_STREAM_CONTROLLER_H
#define DAS_STREAM_CONTROLLER_H

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <Helpz/net_protocol_timer.h>

#include "stream_client.h"

namespace Das {

class Stream_Controller : public Helpz::Net::Protocol_Writer, public Helpz::Net::Protocol_Timer_Emiter
{
public:
    Stream_Controller(boost::asio::io_context& io_context, boost::asio::ip::udp::resolver::query query);

    void on_protocol_timeout(boost::asio::ip::udp::endpoint endpoint, void *data) override;

    void write(const QByteArray &data) override;

    void handle_send(std::unique_ptr<uint8_t[]> &data, std::size_t size, const boost::system::error_code &error, const std::size_t &bytes_transferred);

    void write(Helpz::Net::Message_Item message) override;

    void add_timeout_at(std::chrono::system_clock::time_point time_point, void *data) override;

    std::shared_ptr<Helpz::Net::Protocol> protocol() override;

    std::shared_ptr<Stream_Client> client_;
private:
    void start_receive();
    void handle_receive(const boost::system::error_code& err, std::size_t size);

    Helpz::Net::Protocol_Timer timer_;

    boost::asio::io_context io_context_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint receiver_endpoint_, sender_endpoint_;

    std::unique_ptr<uint8_t[]> recv_buffer_;
};

} // namespace Das

#endif // DAS_STREAM_CONTROLLER_H
