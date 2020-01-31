#ifndef DAS_LOG_SYNCHRONIZER_2_0_H
#define DAS_LOG_SYNCHRONIZER_2_0_H

#include <map>
#include <vector>
#include <ctime>

#include <QIODevice>
#include <QDataStream>

#include <Helpz/db_table.h>
#include <Helpz/db_meta.h>

#include <Das/log/log_type.h>
//#include <Das/log/log_pack.h>

#include "base_synchronizer.h"

namespace Das {

namespace Server {
class Protocol_Base;
} // namespace Server

namespace Ver_2_0 {
namespace Server {

using namespace Das::Server;

#define LOG_SYNC_PACK_MAX_SIZE  200

namespace Old {
class Log_Base_Item
{
public:
    Log_Base_Item(uint32_t id = 0, qint64 time_msecs = 0, uint32_t user_id = 0);
    Log_Base_Item(Log_Base_Item&&) = default;
    Log_Base_Item(const Log_Base_Item&) = default;
    Log_Base_Item& operator =(Log_Base_Item&&) = default;
    Log_Base_Item& operator =(const Log_Base_Item&) = default;

    uint32_t id() const;
    void set_id(uint32_t id);

    qint64 time_msecs() const;
    void set_time_msecs(qint64 time_msecs);

    QVariant date_to_db() const;

    void set_date_from_db(const QVariant& value);

    uint32_t user_id() const;
    void set_user_id(uint32_t user_id);

private:
    uint32_t id_, user_id_;
    qint64 time_msecs_; // Milliseconds

    friend QDataStream & operator>>(QDataStream& ds, Log_Base_Item& item);
};

QDataStream &operator>>(QDataStream &ds, Log_Base_Item &item);
QDataStream &operator<<(QDataStream &ds, const Log_Base_Item &item);

#define DB_DATE date, time_msecs()

class Log_Value_Item : public Old::Log_Base_Item
{
    HELPZ_DB_META(Log_Value_Item, "logs", "hl", 6, DB_A(id), DB_AT(date), DB_AN(user_id), DB_A(item_id), DB_A(raw_value), DB_A(value))
public:
    Log_Value_Item(uint32_t id = 0, qint64 time_msecs = 0, uint32_t user_id = 0, uint32_t item_id = 0,
                   const QVariant& raw_value = {}, const QVariant& value = {});

    Log_Value_Item(Log_Value_Item&&) = default;
    Log_Value_Item(const Log_Value_Item&) = default;
    Log_Value_Item& operator =(Log_Value_Item&&) = default;
    Log_Value_Item& operator =(const Log_Value_Item&) = default;

    uint32_t item_id() const;
    void set_item_id(uint32_t item_id);

    QVariant value() const;
    void set_value(const QVariant& value);

    QVariant raw_value() const;
    void set_raw_value(const QVariant& raw_value);

private:
    uint32_t item_id_;
    QVariant raw_value_, value_;

    friend QDataStream &operator>>(QDataStream& ds, Log_Value_Item& item);
};

QDataStream &operator<<(QDataStream &ds, const Log_Value_Item &item);
QDataStream &operator>>(QDataStream &ds, Log_Value_Item &item);

class Log_Event_Item : public Log_Base_Item
{
    HELPZ_DB_META(Log_Event_Item, "eventlog", "hel", 6, DB_A(id), DB_AT(date), DB_AN(user_id), DB_A(type), DB_A(who), DB_A(msg))
public:
    Log_Event_Item(uint32_t id = 0, qint64 time_msecs = 0, uint32_t user_id = 0, uint8_t type_id = 0,
                   const QString& category = {}, const QString& text = {});

    Log_Event_Item(Log_Event_Item&&) = default;
    Log_Event_Item(const Log_Event_Item&) = default;
    Log_Event_Item& operator =(Log_Event_Item&&) = default;
    Log_Event_Item& operator =(const Log_Event_Item&) = default;

    enum Event_Flags : uint8_t {
        EVENT_NEED_TO_INFORM = 0x80
    };

    uint8_t type() const;
    void set_type(uint8_t type_id);

    QString who() const;
    void set_who(QString category);

    QString msg() const;
    void set_msg(QString text);

    QString toString() const;

    bool need_to_inform() const;
private:
    uint8_t type_id_;
    QString category_, text_;

    friend QDataStream & operator>>(QDataStream& ds, Log_Event_Item& item);
};

QDataStream &operator<<(QDataStream &ds, const Log_Event_Item &item);
QDataStream &operator>>(QDataStream &ds, Log_Event_Item &item);

} // namespace Old

struct LogSyncInfo
{
#define OLD_LOG_SYNC_PACK_MAX_SIZE  250
    const uint16_t pack_max_size = OLD_LOG_SYNC_PACK_MAX_SIZE;
    LogSyncInfo();
    LogSyncInfo& operator =(LogSyncInfo&& o);

    LogSyncInfo(const LogSyncInfo&) = delete;
    LogSyncInfo& operator =(const LogSyncInfo&) = delete;

    bool in_range(uint32_t id) const;
    bool range_received_;

    uint16_t pack_size_;
    uint16_t change_size_;

    uint32_t id;
    qint64 time;
    std::time_t request_time_;
    std::vector<QPair<uint32_t, uint32_t>> ranges_;
};

std::time_t time_since_epoch();

QString old_log_table_name(uint8_t log_type, const QString& db_name);

class Log_Synchronizer : public Base_Synchronizer
{
public:
    Log_Synchronizer(Server::Protocol_Base* protocol);

    void check();

    void check_unsync_log(Log_Type_Wrapper log_type, uint32_t db_id, qint64 time_ms);

    QString valueLogSql(const QString &name);

    QString eventLogSql(const QString &name);

    void deferred_eventLog(const QString &name, const QVector<Old::Log_Event_Item> &pack, std::function<void(uint32_t, qint64)> log_cb);

    void process_values_pack(const QVector<Old::Log_Value_Item>& pack);

    void process_events_pack(const QVector<Old::Log_Event_Item>& pack);

    void setLastLogSyncTime(const QString &name, const LogSyncInfo& info, const QString& paramName);

    void insertUnsyncData(const QString &name, const QVector<uint32_t>& not_found, const QVector<Old::Log_Value_Item>& data, LogSyncInfo *info);
    void insertUnsyncData(const QString &name, const QVector<uint32_t>& not_found, const QVector<Old::Log_Event_Item>& data, LogSyncInfo *info);

    template<typename T>
    void __insertUnsyncData(const QString &name, const QVector<uint32_t>& not_found, const QVector<T>& data, LogSyncInfo *info);

    void deffered_setDevItemValue(const QString &name, const QVector<Old::Log_Value_Item> &pack, std::function<void(uint32_t, qint64)> log_cb);

    QVariantList logValuesFromItem(const Old::Log_Value_Item &item);
    QVariantList logValuesFromItem(const Old::Log_Event_Item &item);
private:
    void request_log_range(uint8_t log_type, qint64 log_time);
    void request_sync_seldom(Log_Type_Wrapper log_type, bool continue_only = false);

    void request_sync(Log_Type_Wrapper log_type);
    void request_sync(LogSyncInfo& info, uint8_t log_type);

    std::vector<QPair<uint32_t, uint32_t>> checkSyncRange(const QString &name, uint8_t log_type, const QPair<uint32_t, uint32_t> &range, qint64 date_ms);

    void process_log_range(Log_Type_Wrapper log_type, const QPair<uint32_t, uint32_t> &range);

    void process_log_data(Log_Type_Wrapper log_type, const QVector<uint32_t> &not_found, QIODevice *data_dev);

    std::map<uint8_t, LogSyncInfo> sync_info_;
};

} // namespace Server
} // namespace Ver_2_0
} // namespace Das

#endif // DAS_LOG_SYNCHRONIZER_2_0_H
