#ifndef DAS_SERVER_LOG_SAVER_DATA_H
#define DAS_SERVER_LOG_SAVER_DATA_H

#include <plus/das/log_saver_def.h>

namespace Das {
namespace Log_Saver {

class Data
{
public:
    enum Type { Log, Cache };
    Type type() const { return _type; }
protected:
    Data(Type type) : _type(type) {}
private:
    Type _type;
};

template<typename T>
class Data_Impl : public Data
{
public:
    Data_Impl(Data::Type type, vector<T>&& data) :
        Data(type),
        _data(move(data))
    {}

    vector<T> _data;
};

} // namespace Log_Saver
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_DATA_H
