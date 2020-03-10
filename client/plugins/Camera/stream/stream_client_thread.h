#ifndef DAS_STREAM_CLIENT_THREAD_H
#define DAS_STREAM_CLIENT_THREAD_H

#include <thread>

#include <boost/asio/io_context.hpp>

#include <QByteArray>

namespace Das {

class Stream_Controller;
class Stream_Client_Thread
{
public:
    Stream_Client_Thread(const std::string &host, const std::string &port);
    ~Stream_Client_Thread();

    void send(uint32_t dev_item_id, const QByteArray &param, const QByteArray& buffer);
private:
    void run();

    std::thread thread_;

    boost::asio::io_context io_context_;

    std::shared_ptr<Stream_Controller> socket_;
};

} // namespace Das

#endif // DAS_STREAM_CLIENT_THREAD_H
