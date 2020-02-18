
#include "database/db_thread_manager.h"
#include "dbus_object.h"
#include "server_protocol_base.h"

namespace Das {
namespace Server {

Q_LOGGING_CATEGORY(Log, "net")
Q_LOGGING_CATEGORY(DetailLog, "net.detail")

Protocol_Base::Protocol_Base(Work_Object* work_object) :
    connection_state_(CS_DISCONNECTED),
    work_object_(work_object)
{
}

QString Protocol_Base::version() const { return version_; }

uint8_t Protocol_Base::connection_state() const
{
    return connection_state_;
}

void Protocol_Base::set_connection_state(uint8_t connection_state)
{
    if (connection_state_ != connection_state)
    {
        connection_state_ = connection_state;
        QMetaObject::invokeMethod(work_object_->dbus_, "connection_state_changed", Qt::QueuedConnection, Q_ARG(Scheme_Info, *this), Q_ARG(uint8_t, connection_state_));
    }
}

bool Protocol_Base::operator ==(const Helpz::Network::Protocol &o) const
{
    return id() > 0 && id() == static_cast<const Protocol_Base &>(o).id();
}

const QString &Protocol_Base::name() const { return name_; }
void Protocol_Base::set_name(const QString &name) { name_ = name; }

const Protocol_Base::TimeInfo &Protocol_Base::time() const { return time_; }
void Protocol_Base::set_time_param(const QTimeZone &zone, qint64 offset)
{
    if (time_.zone != zone)
        time_.zone = zone;
    if (time_.offset != offset)
        time_.offset = offset;
}

QDateTime Protocol_Base::get_value_time(qint64 time_msecs) const
{
    if (time().zone.isValid())
    {
        return QDateTime::fromMSecsSinceEpoch(time_msecs, time().zone);
    }
    else
    {
        return QDateTime::fromMSecsSinceEpoch(time_msecs);
    }
}

std::shared_ptr<DB::global> Protocol_Base::db()
{
    return work_object_->db_thread_mng_->get_db();
}

Helpz::DB::Thread* Protocol_Base::db_thread()
{
    return work_object_->db_thread_mng_->thread();
}

Work_Object* Protocol_Base::work_object()
{
    return work_object_;
}

void Protocol_Base::set_version(const QString& version)
{
    version_ = version;
}

} // namespace Server
} // namespace Das
