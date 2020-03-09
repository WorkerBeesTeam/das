#include "stream_client.h"

namespace Das {

Stream_Client::Stream_Client()
{
}

void Stream_Client::process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    qDebug("Stream_Client::process_message unknown");
}

void Stream_Client::process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    qDebug("Stream_Client::process_answer_message unknown");
}

} // namespace Das
