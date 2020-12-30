
#include "stream_controller.h"

namespace Das {

Q_LOGGING_CATEGORY(StreamLog, "cam.stream")

Stream_Client::Stream_Client(Stream_Controller *controller) :
    controller_(controller)
{
}

void Stream_Client::process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    if (cmd == Helpz::Net::Cmd::CLOSE)
        controller_->close();
    else
        qCDebug(StreamLog) << "Stream_Client::process_message unknown" << cmd << data_dev.size();
}

void Stream_Client::process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    qCDebug(StreamLog) << "Stream_Client::process_answer_message unknown" << cmd << data_dev.size();
}

// ------------------------------------------------------------------------

Stream_Controller::Stream_Controller(boost::asio::io_context &io_context, boost::asio::ip::udp::resolver::query query) :
    timer_(this),
    io_context_(io_context),
    socket_(io_context),
    recv_buffer_(new uint8_t[ HELPZ_MAX_UDP_PACKET_SIZE ])
{
    using boost::asio::ip::udp;

    udp::resolver resolver(io_context_);
    receiver_endpoint_ = *resolver.resolve(query);

    socket_.open(udp::v4());

    client_.reset(new Stream_Client(this));

    start_receive();
}

Stream_Controller::~Stream_Controller()
{
    timer_.stop();
    timer_.join();

    socket_.close();
}

void Stream_Controller::close()
{
    io_context_.stop();
}

void Stream_Controller::on_protocol_timeout(boost::asio::ip::udp::endpoint endpoint, void *data)
{
    Q_UNUSED(endpoint);

    try
    {
        io_context_.post([this, data]()
        {
            client_->process_wait_list(data);
        });
    }
    catch(const std::exception& e)
    {
        qCDebug(StreamLog) << "Stream_Controller::on_protocol_timeout excrption:" << e.what();
    }
    catch(...)
    {
        qCDebug(StreamLog) << "Stream_Controller::on_protocol_timeout excrption!";
    }
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
        qCCritical(StreamLog) << "SEND ERROR" << error.category().name() << ":" << error.message().c_str() << "size:" << size;
    else if (size != bytes_transferred)
        qCCritical(StreamLog) << "SEND ERROR wrong size" << size << "transfered:" << bytes_transferred;
    else if (!data)
        qCCritical(StreamLog) << "SEND ERROR empty data pointer";
}

void Stream_Controller::write(std::shared_ptr<Helpz::Net::Message_Item> message)
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
        qCCritical(StreamLog) << "RECV ERROR" << err.category().name() << ":" << err.message().c_str();
    else
    {
        client_->process_bytes(recv_buffer_.get(), size);
        start_receive();
    }
}

} // namespace Das
