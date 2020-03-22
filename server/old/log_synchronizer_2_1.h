#ifndef DAS_LOG_SYNCHRONIZER_2_1_H
#define DAS_LOG_SYNCHRONIZER_2_1_H

#include <map>
#include <vector>
#include <ctime>

#include <QIODevice>

#include <Helpz/db_table.h>

#include <Das/log/log_type.h>
#include <Das/log/log_pack.h>

#include "base_synchronizer.h"

namespace Das {
namespace Server {
class Protocol_Base;
} // namespace Server

namespace Ver_2_1 {
namespace Server {

using namespace Das::Server;

#define LOG_SYNC_PACK_MAX_SIZE  200

class Log_Sync_Item : public Base_Synchronizer
{
public:
    Log_Sync_Item(Log_Type type, Protocol_Base *protocol);
    virtual ~Log_Sync_Item() = default;

    void init_last_time();
    void check(qint64 request_time);
protected:
    Helpz::DB::Thread* log_thread();
    QString log_insert_sql(const std::shared_ptr<DB::global>& db_ptr, const QString &name) const;

    virtual QString t_str() const = 0;
    virtual Helpz::DB::Table get_log_table(const std::shared_ptr<DB::global>& db_ptr, const QString& name) const = 0;
    virtual QString get_param_name() const = 0;
    virtual void fill_log_data(QIODevice& data_dev, QString& sql, QVariantList& values_pack, int& row_count) = 0;
private:
    void check_next();
    void request_log_range_count();
    bool process_log_range_count(uint32_t remote_count, bool just_check = false);
    void save_sync_time(const std::shared_ptr<DB::global>& db_ptr);
    void request_log_data();
    void process_log_data(QIODevice& data_dev);

private:
    bool in_progress_;
    uint8_t type_;
    qint64 last_time_, request_time_;
};

class Log_Sync_Values final : public Log_Sync_Item
{
public:
    Log_Sync_Values(Protocol_Base *protocol);

    void process_pack(const QVector<Log_Value_Item>& pack);
private:
    QString t_str() const override;
    Helpz::DB::Table get_log_table(const std::shared_ptr<DB::global>& db_ptr, const QString& name) const override;
    QString get_param_name() const override;
    void fill_log_data(QIODevice& data_dev, QString& sql, QVariantList& values_pack, int& row_count) override;
};

class Log_Sync_Events final : public Log_Sync_Item
{
public:
    Log_Sync_Events(Protocol_Base *protocol);

    void process_pack(const QVector<Log_Event_Item>& pack);
private:
    QString t_str() const override;
    Helpz::DB::Table get_log_table(const std::shared_ptr<DB::global>& db_ptr, const QString& name) const override;
    QString get_param_name() const override;
    void fill_log_data(QIODevice& data_dev, QString& sql, QVariantList& values_pack, int& row_count) override;
};

class Log_Synchronizer
{
public:
    Log_Synchronizer(Server::Protocol_Base* protocol);

    void init_last_time();
    void check();

    Log_Sync_Values values_;
    Log_Sync_Events events_;
};

} // namespace Server
} // namespace Ver_2_1
} // namespace Das

#endif // DAS_LOG_SYNCHRONIZER_2_1_H
