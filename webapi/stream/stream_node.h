#ifndef DAS_STREAM_NODE_H
#define DAS_STREAM_NODE_H

#include <set>

#include <boost/asio/ip/udp.hpp>

#include <Helpz/net_protocol_writer.h>

namespace Das {

class Stream_Server_Controller;
class Stream_Server_Protocol;
class Stream_Node : public Helpz::Net::Protocol_Writer
{
public:
    Stream_Node(Stream_Server_Controller* ctrl, boost::asio::ip::udp::endpoint remote_endpoint);
    void set_writer(std::shared_ptr<Helpz::Net::Protocol_Writer> writer);
    void send_close();

    struct Stream_Info {
        uint32_t scheme_id_;
        uint32_t dev_item_id_;

        bool operator<(const Stream_Info& o) const {
            return scheme_id_ < o.scheme_id_ || (scheme_id_ == o.scheme_id_ && dev_item_id_ < o.dev_item_id_);
        }
    };

    std::set<Stream_Info> info_;

    void process_data(uint8_t* data, std::size_t size);
    void process_wait_list(void* data);

    void send_frame(qint64 param, uint32_t dev_item_id, const QByteArray& buffer);
    void send_text(qint64 param, uint32_t dev_item_id, const QString& text);
    void close();
private:
    void write(std::shared_ptr<Helpz::Net::Message_Item> message) override;
    void write(const QByteArray &data) override;
    void handle_send(std::unique_ptr<uint8_t[]> &data, std::size_t size, const boost::system::error_code &error, const std::size_t &bytes_transferred);

    void add_timeout_at(std::chrono::system_clock::time_point time_point, void *data) override;
    std::shared_ptr<Helpz::Net::Protocol> protocol() override;

    boost::asio::ip::udp::endpoint remote_endpoint_;

    std::shared_ptr<Stream_Server_Protocol> proto_;

    Stream_Server_Controller* controller_;
};

} // namespace Das

#endif // DAS_STREAM_NODE_H
