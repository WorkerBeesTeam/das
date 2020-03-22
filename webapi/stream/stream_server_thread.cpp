#include <iostream>

#include <boost/asio/io_context.hpp>

#include <QDataStream>

#include "stream_server.h"
#include "stream_server_thread.h"

namespace Das {

Stream_Server_Thread::Stream_Server_Thread(Net::WebSocket* websock, uint16_t port)
{
    if (port == std::numeric_limits<uint16_t>::max())
        return;

    std::thread th(&Stream_Server_Thread::run, this, websock, port);
    thread_.swap(th);
}

Stream_Server_Thread::~Stream_Server_Thread()
{
    io_context_->stop();
    if (thread_.joinable())
        thread_.join();
}

void Stream_Server_Thread::set_param(uint32_t scheme_id, uint32_t dev_item_id, const QByteArray &data)
{
    qint64 param;
    QDataStream ds(data);
    ds >> param;
    if (ds.status() != QDataStream::Ok)
    {
        std::cerr << "Stream_Server_Thread::set_param bad size " << data.size() << std::endl;
        return;
    }

    io_context_->post([this, param, scheme_id, dev_item_id]() {
        server_->add_stream(param, scheme_id, dev_item_id);
    });
}

void Stream_Server_Thread::remove_stream(uint32_t scheme_id, uint32_t dev_item_id)
{
    io_context_->post([=]() {
        server_->remove_stream(scheme_id, dev_item_id);
    });
}

void Stream_Server_Thread::run(Net::WebSocket* websock, uint16_t port)
{
    try
    {
        boost::asio::io_context io_context;
        io_context_ = &io_context;

        Stream_Server server(io_context, websock, port);
        server_ = &server;
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Stream_Server_Thread exception: " << e.what() << std::endl;
    }
}

} // namespace Das
