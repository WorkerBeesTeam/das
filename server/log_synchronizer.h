#ifndef DAS_LOG_SYNCHRONIZER_H
#define DAS_LOG_SYNCHRONIZER_H

#include <map>
#include <vector>
#include <ctime>

#include <QIODevice>

#include <Helpz/db_table.h>

#include <Das/log/log_type.h>
#include <Das/log/log_pack.h>

#include "base_synchronizer.h"

namespace Das {
namespace Ver {
namespace Server {

using namespace Das::Server;

#define LOG_SYNC_PACK_MAX_SIZE  200

class Log_Sync_Item : public Base_Synchronizer
{
public:
    Log_Sync_Item(Log_Type type, Protocol_Base *protocol);
    virtual ~Log_Sync_Item() = default;

    void check();
    void process_log_data(QIODevice& data_dev, uint8_t msg_id);
protected:
    Helpz::DB::Thread* log_thread();

    virtual QString get_param_name() const = 0;
    virtual void fill_log_data(QIODevice& data_dev, QString& sql, QVariantList& values_pack, int& row_count) = 0;
private:
    void request_log_range_count();
    void request_log_data();

private:
    bool in_progress_;
    Log_Type_Wrapper type_;
};

class Log_Sync_Values final : public Log_Sync_Item
{
public:
    Log_Sync_Values(Protocol_Base *protocol);

    void process_pack(QVector<Log_Value_Item>&& pack, uint8_t msg_id);
private:
    QString get_param_name() const override;
    void fill_log_data(QIODevice& data_dev, QString& sql, QVariantList& values_pack, int& row_count) override;
};

class Log_Sync_Events final : public Log_Sync_Item
{
public:
    Log_Sync_Events(Protocol_Base *protocol);

    void process_pack(QVector<Log_Event_Item> &&pack, uint8_t msg_id);
private:
    QString get_param_name() const override;
    void fill_log_data(QIODevice& data_dev, QString& sql, QVariantList& values_pack, int& row_count) override;
};

class Log_Synchronizer
{
public:
    Log_Synchronizer(Protocol_Base* protocol);

    void check();

    void process_data(Log_Type_Wrapper type_id, QIODevice* data_dev, uint8_t msg_id);
    void process_pack(Log_Type_Wrapper type_id, QIODevice* data_dev, uint8_t msg_id);
    Log_Sync_Item* log_sync_item(uint8_t type_id);
    void request_log_data(uint8_t type_id);

    Log_Sync_Values values_;
    Log_Sync_Events events_;
};

} // namespace Server
} // namespace Ver
} // namespace Das

#endif // DAS_LOG_SYNCHRONIZER_H
