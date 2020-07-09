#ifndef DAS_STREAM_SERVER_CONTROLLER_H
#define DAS_STREAM_SERVER_CONTROLLER_H

#include <Helpz/net_protocol_timer.h>

#include <QByteArray>

namespace Das {

class Stream_Server_Controller : public Helpz::Net::Protocol_Timer_Emiter
{
public:
    Stream_Server_Controller(boost::asio::io_context& io_context, uint16_t port);
    virtual ~Stream_Server_Controller() = default;

    virtual void send_frame(boost::asio::ip::udp::endpoint remote_endpoint, qint64 param, uint32_t dev_item_id, const QByteArray& buffer) = 0;
    virtual void send_text(boost::asio::ip::udp::endpoint remote_endpoint, qint64 param, uint32_t dev_item_id, const QString& text) = 0;
    virtual void remove(boost::asio::ip::udp::endpoint remote_endpoint) = 0;

    boost::asio::io_context& io_context_;
    boost::asio::ip::udp::socket socket_;
    Helpz::Net::Protocol_Timer timer_;
};

} // namespace Das

#endif // DAS_STREAM_SERVER_CONTROLLER_H
