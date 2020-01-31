#ifndef DAS_PROTOCOL_LATEST_H
#define DAS_PROTOCOL_LATEST_H

#include <Das/scheme.h>
#include <Das/db/dig_status.h>

#include "log_sender.h"
#include "structure_synchronizer.h"
#include "client_protocol.h"

namespace Das {

class Scripted_Scheme;

namespace Ver_2_4 {
namespace Client {

using namespace Das::Client;

class Protocol : public Protocol_Base
{
public:
    Protocol(Worker* worker, const Authentication_Info &auth_info);
    virtual ~Protocol();

    Log_Sender& log_sender();
    Structure_Synchronizer& structure_sync();

    void send_mode(uint32_t user_id, uint mode_id, uint32_t group_id);
    void send_status_added(uint32_t group_id, uint32_t info_id, const QStringList& args, uint32_t);
    void send_status_removed(uint32_t group_id, uint32_t info_id, uint32_t);
    void send_dig_param_values(uint32_t user_id, const QVector<DIG_Param_Value>& pack);

//    bool modify_scheme(uint32_t user_id, quint8 struct_type, QIODevice* data_dev);
private:
    QVector<Das::DIG_Status> get_group_statuses() const;

    void restart(uint32_t user_id);
    void write_to_item(uint32_t user_id, uint32_t item_id, const QVariant& raw_data);
    void set_mode(uint32_t user_id, uint32_t mode_id, uint32_t group_id);
    void set_dig_param_values(uint32_t user_id, const QVector<DIG_Param_Value>& pack);
    void exec_script_command(uint32_t user_id, const QString& script, bool is_function, const QVariantList& arguments);

    void send_scheme_structure(uint8_t struct_type, uint8_t msg_id, QIODevice* data_dev, Helpz::Network::Protocol* protocol);

    void ready_write() override;
    void process_message(uint8_t msg_id, uint8_t cmd, QIODevice& data_dev) override;
    void process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice& data_dev) override;

    void parse_script_command(uint32_t user_id, const QString& script, QIODevice* data_dev);
    void process_item_file(QIODevice &data_dev);

    void start_authentication();
    void process_authentication(bool authorized, const QUuid &connection_id);
    void send_version(uint8_t msg_id);
    void send_time_info(uint8_t msg_id);

    Scripted_Scheme* prj_;

    Log_Sender log_sender_;
    Structure_Synchronizer structure_sync_;
};

} // namespace Client
} // namespace Ver_2_4
} // namespace Das

#endif // DAS_PROTOCOL_LATEST_H
