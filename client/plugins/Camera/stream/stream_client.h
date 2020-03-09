#ifndef DAS_STREAM_CLIENT_H
#define DAS_STREAM_CLIENT_H

#include <Helpz/net_protocol.h>

namespace Das {

class Stream_Client : public Helpz::Net::Protocol
{
public:
    Stream_Client();

private:
    void process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev) override;
    void process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev) override;
};

} // namespace Das

#endif // DAS_STREAM_CLIENT_H
