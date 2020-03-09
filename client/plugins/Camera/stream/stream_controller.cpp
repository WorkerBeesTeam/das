#include "stream_controller.h"

namespace Das {

Stream_Controller::Stream_Controller(boost::asio::io_context &io_context, boost::asio::ip::udp::resolver::query query) :
    timer_(this),
    socket_(io_context),
    recv_buffer_(new uint8_t[ HELPZ_MAX_UDP_PACKET_SIZE ])
{
    using boost::asio::ip::udp;

    udp::resolver resolver(io_context_);
    receiver_endpoint_ = *resolver.resolve(query);

    socket_.open(udp::v4());

    client_.reset(new Stream_Client);

    start_receive();
}

void Stream_Controller::on_protocol_timeout(boost::asio::ip::udp::endpoint endpoint, void *data)
{
    Q_UNUSED(endpoint);

    io_context_.post([this, data]()
    {
        client_->process_wait_list(data);
    });
}

void Stream_Controller::write(const QByteArray &data)
{
    std::size_t size = data.size();
    std::unique_ptr<uint8_t[]> send_buffer(new uint8_t[ size ]);
    memcpy(send_buffer.get(), data.constData(), size);
    auto buffer = boost::asio::buffer(send_buffer.get(), size);

    socket_.async_send_to(buffer, receiver_endpoint_,
                          std::bind(&Stream_Controller::handle_send, this,
                                    std::move(send_buffer), size,
                                    std::placeholders::_1,   // boost::asio::placeholders::error,
                                    std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void Stream_Controller::handle_send(std::unique_ptr<uint8_t[]> &data, std::size_t size, const boost::system::error_code &error, const std::size_t &bytes_transferred)
{
    if (error.value() != 0)
        std::cerr << "SEND ERROR " << error.category().name() << ": " << error.message() << " size: " << size << std::endl;
    else if (size != bytes_transferred)
        std::cerr << "SEND ERROR wrong size " << size << " transfered: " << bytes_transferred << std::endl;
    else if (!data)
        std::cerr << "SEND ERROR empty data pointer" << std::endl;
}

void Stream_Controller::write(Helpz::Net::Message_Item message)
{
    const QByteArray data = client_->prepare_packet_to_send(std::move(message));
    write(data);
}

void Stream_Controller::add_timeout_at(std::chrono::system_clock::time_point time_point, void *data)
{
    timer_.add(time_point, boost::asio::ip::udp::endpoint(), data);
}

std::shared_ptr<Helpz::Net::Protocol> Stream_Controller::protocol() { return client_; }

void Stream_Controller::start_receive()
{
    socket_.async_receive_from(
                boost::asio::buffer(recv_buffer_.get(), HELPZ_MAX_UDP_PACKET_SIZE), sender_endpoint_,
                std::bind(&Stream_Controller::handle_receive, this,
                          std::placeholders::_1,   // boost::asio::placeholders::error,
                          std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void Stream_Controller::handle_receive(const boost::system::error_code &err, std::size_t size)
{
    if (err)
    {
        std::cerr << "Stream_Server::handle_receive RECV ERROR " << err.category().name() << ": " << err.message() << std::endl;
        return;
    }

//    std::cout << "RECV " << sender_endpoint_ << " size " << size << std::endl;

    client_->process_bytes(recv_buffer_.get(), size);

    start_receive();
}

} // namespace Das
