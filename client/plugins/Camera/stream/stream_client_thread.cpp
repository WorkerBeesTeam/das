#include <iostream>

#include "stream_controller.h"
#include "stream_client_thread.h"

namespace Das {

Stream_Client_Thread::Stream_Client_Thread(const std::string &host, const std::string &port) :
    io_context_()
{
    using boost::asio::ip::udp;
    udp::resolver::query query(udp::v4(), host, port);

    socket_ = std::make_shared<Stream_Controller>(io_context_, query);
    socket_->client_->set_writer(socket_);
//    client_->set_writer();


    std::thread th(&Stream_Client_Thread::run, this);
    thread_.swap(th);
}

Stream_Client_Thread::~Stream_Client_Thread()
{
    io_context_.stop();
    if (thread_.joinable())
        thread_.join();
}

void Stream_Client_Thread::send(uint32_t dev_item_id, const QByteArray& param, const QByteArray &buffer)
{
    auto sender = socket_->client_->send(Helpz::Net::Cmd::USER_COMMAND);
    sender.set_fragment_size(HELPZ_MAX_PACKET_DATA_SIZE);
    sender.writeRawData(param.constData(), param.size());
    sender << dev_item_id << buffer;
//    socket_.send_to(boost::asio::buffer(buffer.constData(), std::min(buffer.size(), 65507)), receiver_endpoint_);
}

void Stream_Client_Thread::run()
{
    try
    {
        io_context_.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Stream_Client_Thread: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Stream_Client_Thread exception" << std::endl;
    }
}

} // namespace Das
