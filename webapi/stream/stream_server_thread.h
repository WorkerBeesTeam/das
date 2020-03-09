#ifndef DAS_STREAM_SERVER_THREAD_H
#define DAS_STREAM_SERVER_THREAD_H

#include <thread>

namespace boost::asio {
class io_context;
}

namespace Das {

namespace Net {
class WebSocket;
}

class Stream_Server;
class Stream_Server_Thread
{
public:
    Stream_Server_Thread(Net::WebSocket* websock, uint16_t port);
    ~Stream_Server_Thread();
private:
    void run(Net::WebSocket* websock, uint16_t port);

    boost::asio::io_context* io_context_;

    std::unique_ptr<Stream_Server> server_;
    std::thread thread_;
};

} // namespace Das

#endif // DAS_STREAM_SERVER_THREAD_H
