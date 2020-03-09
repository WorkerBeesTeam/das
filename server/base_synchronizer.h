#ifndef DAS_BASE_SYNCHRONIZER_H
#define DAS_BASE_SYNCHRONIZER_H

#include <Helpz/net_protocol.h>
#include <Helpz/db_thread.h>

namespace Das {
namespace Server {
class Protocol_Base;
} // namespace Server

namespace DB {
class global;
} // namespace DB

class Base_Synchronizer
{
public:
    enum { DATASTREAM_VERSION = Helpz::Network::Protocol::DATASTREAM_VERSION };

    Base_Synchronizer(Server::Protocol_Base* protocol);
    virtual ~Base_Synchronizer() = default;

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(QIODevice& data_dev, RetType(T::*__f)(FArgs...) const, Args&&... args)
    {
        std::unique_ptr<QDataStream> ds = Helpz::parse_open_device(data_dev, DATASTREAM_VERSION);
        return Helpz::apply_parse_impl<RetType, decltype(__f), T, FArgs...>(*ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(QIODevice& data_dev, RetType(T::*__f)(FArgs...), Args&&... args)
    {
        std::unique_ptr<QDataStream> ds = Helpz::parse_open_device(data_dev, DATASTREAM_VERSION);
        return Helpz::apply_parse_impl<RetType, decltype(__f), T, FArgs...>(*ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    Server::Protocol_Base *protocol();

protected:
    uint32_t scheme_id() const;
    QString title() const;
    std::shared_ptr<DB::global> db();

    Server::Protocol_Base* protocol_;
};

} // namespace Das

#endif // DAS_BASE_SYNCHRONIZER_H
