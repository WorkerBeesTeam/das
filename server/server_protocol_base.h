#ifndef DAS_SERVER_PROTOCOL_BASE_H
#define DAS_SERVER_PROTOCOL_BASE_H

#include <QLoggingCategory>

#include <Helpz/net_protocol.h>
#include <Helpz/db_thread.h>

#include <plus/das/scheme_info.h>

#include "worker.h"

namespace Das {

namespace DB {
class global;
class Thread_Manager;
}
namespace Server {

Q_DECLARE_LOGGING_CATEGORY(Log)
Q_DECLARE_LOGGING_CATEGORY(DetailLog)

class Protocol_Base : public Helpz::Net::Protocol, public Scheme_Info
{
public:
    Protocol_Base(Worker *work_object);
    virtual ~Protocol_Base() = default;

    virtual int protocol_version() const = 0;
    virtual void send_file(uint32_t user_id, uint32_t dev_item_id, const QString& file_name, const QString& file_path) = 0;

    QString version() const;

    uint8_t connection_state() const;
    void set_connection_state(uint8_t connection_state);

    virtual void synchronize(bool full = false) = 0;

    bool operator ==(const Helpz::Net::Protocol& o) const override;

    const QString &name() const;
    void set_name(const QString& name);

    struct TimeInfo {
        QTimeZone zone;
        qint64 offset = 0;
    };

    const TimeInfo& time() const;
    void set_time_param(const QTimeZone& zone, qint64 offset);

    QDateTime get_value_time(qint64 time_msecs) const;

    std::shared_ptr<DB::global> db();
    Helpz::DB::Thread* db_thread();

    Worker* work_object();
protected:
    void set_version(const QString& version);
private:
    QString name_, version_;
    TimeInfo time_;
    uint32_t connection_state_;
    Worker* work_object_;
};

} // namespace Server
} // namespace Das

#endif // DAS_SERVER_PROTOCOL_BASE_H
