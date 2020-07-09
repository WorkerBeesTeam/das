#include <QDebug>

#include <Helpz/net_protocol.h>

#include "stream_server_controller.h"
#include "stream_node.h"

namespace Das {

class Stream_Server_Protocol : public Helpz::Net::Protocol
{
public:
    Stream_Server_Protocol(Stream_Node *node) :
        node_(node)
    {
    }
private:

    enum Comands {
        CMD_FRAME = Helpz::Net::Cmd::USER_COMMAND,
        CMD_TEXT
    };

    void process_frame(qint64 param, uint32_t dev_item_id, const QByteArray& buffer)
    {
        node_->send_frame(param, dev_item_id, buffer);
    }

    void process_text(qint64 param, uint32_t dev_item_id, const QString& text)
    {
        node_->send_text(param, dev_item_id, text);
    }

    void process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev) override
    {
        try
        {
            if (cmd == CMD_FRAME)
                apply_parse(data_dev, &Stream_Server_Protocol::process_frame);
            else if (cmd == CMD_TEXT)
                apply_parse(data_dev, &Stream_Server_Protocol::process_text);
            else
                qDebug() << "Stream_Server_Protocol::process_message unknown command" << cmd << "size" << data_dev.size();
        }
        catch(...)
        {
            qDebug() << "Failed process_message";
            node_->close();
        }
    }
    void process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev) override
    {
        qDebug("Stream_Server_Protocol::process_answer_message");
    }

    Stream_Node *node_;
};

Stream_Node::Stream_Node(Stream_Server_Controller *ctrl, boost::asio::ip::udp::endpoint remote_endpoint) :
    remote_endpoint_(remote_endpoint),
    controller_(ctrl)
{
    proto_ = std::make_shared<Stream_Server_Protocol>(this);
}

void Stream_Node::set_writer(std::shared_ptr<Helpz::Net::Protocol_Writer> writer)
{
    proto_->set_writer(writer);
}

void Stream_Node::send_close()
{
    proto_->send(Helpz::Net::Cmd::CLOSE);
}

void Stream_Node::process_data(uint8_t *data, std::size_t size)
{
    proto_->process_bytes(data, size);
}

void Stream_Node::process_wait_list(void *data)
{
    proto_->process_wait_list(data);
}

void Stream_Node::send_frame(qint64 param, uint32_t dev_item_id, const QByteArray &buffer)
{
    controller_->send_frame(remote_endpoint_, param, dev_item_id, buffer);
}

void Stream_Node::send_text(qint64 param, uint32_t dev_item_id, const QString &text)
{
    controller_->send_text(remote_endpoint_, param, dev_item_id, text);
}

void Stream_Node::close()
{
    controller_->remove(remote_endpoint_);
}

void Stream_Node::write(std::shared_ptr<Helpz::Net::Message_Item> message)
{
    const QByteArray data = proto_->prepare_packet_to_send(std::move(message));
    write(data);
}

void Stream_Node::write(const QByteArray &data)
{
    std::size_t size = data.size();
    std::unique_ptr<uint8_t[]> send_buffer(new uint8_t[ size ]);
    memcpy(send_buffer.get(), data.constData(), size);
    auto buffer = boost::asio::buffer(send_buffer.get(), size);

    controller_->socket_.async_send_to(buffer, remote_endpoint_,
                          std::bind(&Stream_Node::handle_send, this,
                                    std::move(send_buffer), size,
                                    std::placeholders::_1,   // boost::asio::placeholders::error,
                                    std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void Stream_Node::handle_send(std::unique_ptr<uint8_t[]> &data, std::size_t size, const boost::system::error_code &error, const std::size_t &bytes_transferred)
{
    if (error.value() != 0)
        std::cerr << "SEND ERROR " << error.category().name() << ": " << error.message() << " size: " << size << std::endl;
    else if (size != bytes_transferred)
        std::cerr << "SEND ERROR wrong size " << size << " transfered: " << bytes_transferred << std::endl;
    else if (!data)
        std::cerr << "SEND ERROR empty data pointer" << std::endl;
}

void Stream_Node::add_timeout_at(std::chrono::system_clock::time_point time_point, void *data)
{
    controller_->timer_.add(time_point, remote_endpoint_, data);
}

std::shared_ptr<Helpz::Net::Protocol> Stream_Node::protocol() { return proto_; }

} // namespace Das
