#include <iostream>

#include <boost/asio/io_context.hpp>

#include "stream_server.h"
#include "stream_server_thread.h"

namespace Das {

Stream_Server_Thread::Stream_Server_Thread(Net::WebSocket* websock, uint16_t port)
{
    std::thread th(&Stream_Server_Thread::run, this, websock, port);
    thread_.swap(th);
}

Stream_Server_Thread::~Stream_Server_Thread()
{
    io_context_->stop();
    thread_.join();
}

void Stream_Server_Thread::run(Net::WebSocket* websock, uint16_t port)
{
    try
    {
        boost::asio::io_context io_context;
        io_context_ = &io_context;

        Stream_Server server(io_context, websock, port);
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Stream_Server_Thread exception: " << e.what() << std::endl;
    }
}

} // namespace Das
