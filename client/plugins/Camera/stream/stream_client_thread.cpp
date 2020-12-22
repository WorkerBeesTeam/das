#include <iostream>

#include "stream_controller.h"
#include "stream_client_thread.h"

namespace Das {

Stream_Client_Thread::Stream_Client_Thread(const string &host, const string &port, function<void()> finished_func) :
    _io_context{},
    _finished_func{move(finished_func)}
{
    using boost::asio::ip::udp;
    udp::resolver::query query(udp::v4(), host, port);

    _socket = make_shared<Stream_Controller>(_io_context, query);
    _socket->client_->set_writer(_socket);

    thread th(&Stream_Client_Thread::run, this);
    _thread.swap(th);
}

Stream_Client_Thread::~Stream_Client_Thread()
{
    _socket->client_->set_writer(nullptr);
    _socket.reset();

    _io_context.stop();
    if (_thread.joinable())
        _thread.join();
}

bool Stream_Client_Thread::is_frame_sended(uint32_t dev_item_id) const
{
    lock_guard lock(_mutex);
    auto it = _frame_sended.find(dev_item_id);
    return it == _frame_sended.cend() || it->second;
}

void Stream_Client_Thread::send(uint32_t dev_item_id, const QByteArray& param, const QByteArray &buffer, uint32_t timeout_ms)
{
    {
        lock_guard lock(_mutex);
        auto it = _frame_sended.find(dev_item_id);
        if (it == _frame_sended.end())
            it = _frame_sended.emplace(dev_item_id, false).first;
        else if (!it->second)
            return;
        else
            it->second = false;
    }

    chrono::milliseconds timeout{timeout_ms};

    auto sender = _socket->client_->send(CMD_FRAME);
    sender.timeout(nullptr, timeout, timeout);
    sender.finally([this, dev_item_id](bool)
    {
        lock_guard lock(_mutex);
        _frame_sended[dev_item_id] = true;
    });
    sender.set_fragment_size(HELPZ_MAX_PACKET_DATA_SIZE);
    sender.writeRawData(param.constData(), param.size());
    sender << dev_item_id << buffer;
}

void Stream_Client_Thread::send_text(uint32_t dev_item_id, const QByteArray &param, const QString &text)
{
    auto sender = _socket->client_->send(CMD_TEXT);
    sender.writeRawData(param.constData(), param.size());
    sender << dev_item_id << text;
}

void Stream_Client_Thread::run()
{
    try
    {
        _io_context.run();
    }
    catch (exception& e)
    {
        qCCritical(StreamLog) << "Stream_Client_Thread:" << e.what();
    }
    catch(...)
    {
        qCCritical(StreamLog) << "Stream_Client_Thread exception";
    }

    if (_finished_func)
    {
        _finished_func();
        _finished_func = nullptr;
    }
}

} // namespace Das
