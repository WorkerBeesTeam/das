#include <iostream>
#include <istream>
#include <ostream>

#include <boost/asio/write.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#include "smtp_client.h"

namespace Das {

SMTP_Client::SMTP_Client(const std::string& server, uint16_t port, const std::string& user, const std::string& password) :
    server_(server), username_(user), password_(password), port_(port), io_context_(), resolver_(io_context_), socket_(io_context_)
{
    tcp::resolver::query qry(server_, std::to_string(port_));
    resolver_.async_resolve(qry, boost::bind(&SMTP_Client::handle_resolve, this, boost::asio::placeholders::error,
     boost::asio::placeholders::iterator));
}

bool SMTP_Client::send(const std::string& from, const std::string& to, const std::string& subject, const std::string& message)
{
    from_ = from;
    to_ = to;
    subject_ = subject;
    message_ = message;
    io_context_.run();
    return has_error_;
}

std::string SMTP_Client::error_text() const
{
    return error_msg_;
}

std::string SMTP_Client::encode_base64(const std::string& data)
{
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    std::string tmp(It(std::begin(data)), It(std::end(data)));
    return tmp.append((3 - data.size() % 3) % 3, '=');
}

void SMTP_Client::handle_resolve(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator)
{
    if(!err)
    {
        tcp::endpoint endpoint = *endpoint_iterator;
        std::cerr << "Resolved " << server_ << ':' << port_ << " to " << endpoint.address() << ':' << endpoint.port() << std::endl;
        socket_.async_connect(endpoint,
                             boost::bind(&SMTP_Client::handle_connect, this, boost::asio::placeholders::error, ++endpoint_iterator));
    }
    else
    {
        has_error_ = true;
        error_msg_ = err.message();
    }
}

void SMTP_Client::write_line(const std::string& data)
{
    std::cerr << "WRITE: " << data << std::endl;
    std::ostream req_strm(&request_);
    req_strm << data << "\r\n";
    boost::asio::write(socket_, request_);
    req_strm.clear();
}

void SMTP_Client::handle_connect(const boost::system::error_code& err, tcp::resolver::iterator /*endpoint_iterator*/)
{
    if (!err)
    {
        // The connection was successful. Send the request.
        std::ostream req_strm(&request_);
        write_line("EHLO " + server_);
        write_line("AUTH LOGIN");
        write_line(encode_base64(username_));
        write_line(encode_base64(password_));
        write_line("MAIL FROM:<" + from_ + ">");
        write_line("RCPT TO:<" + to_ + ">");
        write_line("DATA");
        write_line("SUBJECT:" + subject_);
        write_line("From:" + from_);
        write_line("To:" + to_);
        write_line("");
        write_line(message_);
        write_line(".\r\n");
    }
    else
    {
        has_error_ = true;
        error_msg_ = err.message();
    }
}

} // namespace Das
