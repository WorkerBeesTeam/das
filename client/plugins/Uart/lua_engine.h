#ifndef DAS_LUA_ENGINE_H
#define DAS_LUA_ENGINE_H

#include <string>

#include <QVariant>

typedef struct lua_State lua_State;

namespace Das {

class Lua_Engine
{
public:
    ~Lua_Engine();

    void init();
    void stop();

    operator bool() const;
    QVariant operator() (const QByteArray& send_data, const QByteArray& recv_data,
                         const std::string& func_name = "process",
                         const QVariant& arg = {});
private:
    bool get_process_func(const std::string& func_name);
    void push_bytearray(const QByteArray& data);
    void push_variant(const QVariant& data);
    QVariant to_variant(int n);
    std::string get_error_msg();

    lua_State* _lua = nullptr;
};

} // namespace Das

#endif // DAS_LUA_ENGINE_H
