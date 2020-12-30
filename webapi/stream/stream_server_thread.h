#ifndef DAS_STREAM_SERVER_THREAD_H
#define DAS_STREAM_SERVER_THREAD_H

#include <thread>

#include <QByteArray>

namespace boost::asio {
class io_context;
}

namespace Das {

namespace Net {
class WebSocket;
}

struct Stream_Config
{
    Stream_Config() = default;
    Stream_Config(uint16_t port, QString connect_url);
    static Stream_Config& instance();

    uint16_t _port = 6731;
    QString _connect_url = "udp://deviceaccess.ru:6731";
};

class Stream_Server;
class Stream_Server_Thread
{
public:
    Stream_Server_Thread(Net::WebSocket* websock);
    ~Stream_Server_Thread();

    void set_param(uint32_t scheme_id, uint32_t dev_item_id, const QByteArray& data);
    void remove_stream(uint32_t scheme_id, uint32_t dev_item_id);
private:
    void run(Net::WebSocket* websock);

    boost::asio::io_context* io_context_ = nullptr;

    Stream_Server* server_;
    std::thread thread_;
};

} // namespace Das

#endif // DAS_STREAM_SERVER_THREAD_H
