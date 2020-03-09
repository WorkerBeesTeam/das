#include <iostream>

#include <boost/asio/io_context.hpp>

#include <QDebug>

#include <Helpz/net_defs.h>

#include "stream_server.h"

#include "websocket.h"

namespace Das {

using boost::asio::ip::udp;

Stream_Server::Stream_Server(boost::asio::io_context &io_context, Net::WebSocket* websock, uint16_t port) :
    Stream_Server_Controller(io_context, port),
    cleaning_timeout_{30},
    cleaning_timer_{io_context, cleaning_timeout_},
    recv_buffer_(new uint8_t[ HELPZ_MAX_UDP_PACKET_SIZE ]),
    websock_(websock)
{
    cleaning_timer_.async_wait(std::bind(&Stream_Server::cleaning, this, std::placeholders::_1));
    start_receive();
}

Stream_Server::~Stream_Server()
{
    delete[] recv_buffer_;
}

void Stream_Server::add_stream(qint64 param, uint32_t scheme_id, uint32_t dev_item_id)
{
    const Stream_Info info{scheme_id, dev_item_id, {}};
    auto it = stream_map_.emplace(param, info);
    if (!it.second)
    {
        erase_info(it.first->second);
        it.first->second = info;
    }
}

void Stream_Server::remove_stream(uint32_t scheme_id, uint32_t dev_item_id)
{
    for (auto it = stream_map_.begin(); it != stream_map_.end();)
    {
        if (it->second.scheme_id_ == scheme_id && it->second.dev_item_id_ == dev_item_id)
        {
            erase_info(it->second);
            it = stream_map_.erase(it);
        }
        else
            ++it;
    }
}

void Stream_Server::erase_info(const Stream_Server::Stream_Info &info)
{
    for (const udp::endpoint& endpoint: info.endpoints_)
    {
        auto it = clients_.find(endpoint);
        if (it != clients_.end())
        {
            it->second->info_.erase(static_cast<const Stream_Node::Stream_Info&>(info));
            if (it->second->info_.empty())
            {
                it->second->send_close();
                clients_.erase(it);
            }
        }
    }
}

void Stream_Server::start_receive()
{
    auto buffer = boost::asio::buffer(recv_buffer_, HELPZ_MAX_UDP_PACKET_SIZE);

    socket_.async_receive_from(
                buffer, remote_endpoint_,
                std::bind(&Stream_Server::handle_receive, this,
                          std::placeholders::_1,   // boost::asio::placeholders::error,
                          std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void Stream_Server::handle_receive(const boost::system::error_code &err, std::size_t size)
{
    if (err)
    {
        std::cerr << "Stream_Server::handle_receive RECV ERROR " << err.category().name() << ": " << err.message() << std::endl;
        return;
    }

//    std::cout << "RECV " << remote_endpoint_ << " size " << size << std::endl;
    auto it = clients_.find(remote_endpoint_);
    if (it == clients_.cend())
    {
        auto node = std::make_shared<Stream_Node>(this, remote_endpoint_);
        node->set_writer(node);
        it = clients_.emplace(remote_endpoint_, std::move(node)).first;
    }

    it->second->process_data(recv_buffer_, size);

    start_receive();
}

void Stream_Server::on_protocol_timeout(udp::endpoint endpoint, void *data)
{
    io_context_.post([=]()
    {
        auto it = clients_.find(endpoint);
        if (it != clients_.cend())
            it->second->process_wait_list(data);
    });
}

void Stream_Server::send_frame(udp::endpoint remote_endpoint, qint64 param, uint32_t dev_item_id, const QByteArray &buffer)
{
    auto it = stream_map_.find(param);
    if (it != stream_map_.cend() && it->second.dev_item_id_ == dev_item_id)
    {
        if (it->second.endpoints_.insert(remote_endpoint).second)
        {
            auto cl_it = clients_.find(remote_endpoint);
            if (cl_it != clients_.cend())
                cl_it->second->info_.insert(it->second);
        }

        QMetaObject::invokeMethod(websock_, "send_stream_id_data", Qt::QueuedConnection,
                                  Q_ARG(uint32_t, it->second.scheme_id_), Q_ARG(uint32_t, it->second.dev_item_id_), Q_ARG(QByteArray, buffer));
    }
    else
        remove(remote_endpoint);
}

void Stream_Server::remove(udp::endpoint remote_endpoint)
{
    auto it = clients_.find(remote_endpoint);
    if (it == clients_.end())
        return;

    it->second->send_close();
    clients_.erase(it);
}

void Stream_Server::cleaning(const boost::system::error_code &err)
{
    if (err)
    {
        std::cerr << "cleaning timer error " << err << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();

    for(auto it = clients_.begin(); it != clients_.end();)
    {
        if ((now - it->second->last_msg_recv_time()) > cleaning_timeout_)
        {
//            qDebug().noquote() << it->first << "timeout. Erase it.";
            it = clients_.erase(it);
        }
        else
            ++it;
    }

    cleaning_timer_.expires_at(cleaning_timer_.expiry() + cleaning_timeout_);
    cleaning_timer_.async_wait(std::bind(&Stream_Server::cleaning, this, std::placeholders::_1));
}

} // namespace Das
