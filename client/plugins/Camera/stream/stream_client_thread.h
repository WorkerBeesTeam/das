#ifndef DAS_STREAM_CLIENT_THREAD_H
#define DAS_STREAM_CLIENT_THREAD_H

#include <thread>
#include <functional>

#include <boost/asio/io_context.hpp>

#include <QByteArray>

#include <Helpz/net_protocol.h>

namespace Das {

using namespace std;

class Stream_Controller;
class Stream_Client_Thread
{
public:
    Stream_Client_Thread(const string &host, const string &port, function<void()> finished_func);
    ~Stream_Client_Thread();

    bool is_frame_sended(uint32_t dev_item_id) const;

    void send(uint32_t dev_item_id, const QByteArray &param, const QByteArray& buffer, uint32_t timeout_ms);
    void send_text(uint32_t dev_item_id, const QByteArray& param, const QString &text);
private:
    void run();

    enum Comands {
        CMD_FRAME = Helpz::Net::Cmd::USER_COMMAND,
        CMD_TEXT
    };

    map<uint32_t, bool> _frame_sended;

    mutable mutex _mutex;
    thread _thread;

    boost::asio::io_context _io_context;

    shared_ptr<Stream_Controller> _socket;
    function<void()> _finished_func;
};

} // namespace Das

#endif // DAS_STREAM_CLIENT_THREAD_H
