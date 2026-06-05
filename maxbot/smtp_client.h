#ifndef DAS_SMTP_CLIENT_H
#define DAS_SMTP_CLIENT_H

#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

namespace Das {

class SMTP_Client
{
public:
    SMTP_Client(const std::string& server, uint16_t port, const std::string& user, const std::string& password);

    bool send(const std::string& from, const std::string& to, const std::string& subject, const std::string& message);

    std::string error_text() const;
private:
    using tcp = boost::asio::ip::tcp;

    std::string encode_base64(const std::string& data);
    void handle_resolve(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);
    void write_line(const std::string& data);
    void handle_connect(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);

    std::string server_;
    std::string username_;
    std::string password_;
    std::string from_;
    std::string to_;
    std::string subject_;
    std::string message_;
    uint16_t port_;

    boost::asio::io_context io_context_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    bool has_error_;
    std::string error_msg_;
};

} // namespace Das

#endif // SMTP_CLIENT_H
