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

    std::thread th(&Stream_Client_Thread::run, this);
    thread_.swap(th);
}

Stream_Client_Thread::~Stream_Client_Thread()
{
    socket_->client_->set_writer(nullptr);
    socket_.reset();

    io_context_.stop();
    if (thread_.joinable())
        thread_.join();
}

bool Stream_Client_Thread::is_frame_sended(uint32_t dev_item_id) const
{
    auto it = _frame_sended.find(dev_item_id);
    return it == _frame_sended.cend() || it->second;
}

void Stream_Client_Thread::send(uint32_t dev_item_id, const QByteArray& param, const QByteArray &buffer, uint32_t timeout_ms)
{
    auto it = _frame_sended.find(dev_item_id);
    if (it == _frame_sended.end())
        it = _frame_sended.emplace(dev_item_id, false).first;
    else if (!it->second)
        return;
    else
        it->second = false;

    std::chrono::milliseconds timeout{timeout_ms};

    auto sender = socket_->client_->send(CMD_FRAME);
    sender.timeout(nullptr, timeout, timeout);
    sender.finally([this, dev_item_id](bool) { _frame_sended[dev_item_id] = true; });
    sender.set_fragment_size(HELPZ_MAX_PACKET_DATA_SIZE);
    sender.writeRawData(param.constData(), param.size());
    sender << dev_item_id << buffer;
}

void Stream_Client_Thread::send_text(uint32_t dev_item_id, const QByteArray &param, const QString &text)
{
    auto sender = socket_->client_->send(CMD_TEXT);
    sender.writeRawData(param.constData(), param.size());
    sender << dev_item_id << text;
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
