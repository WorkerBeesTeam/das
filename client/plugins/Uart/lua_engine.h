#ifndef DAS_LUA_ENGINE_H
#define DAS_LUA_ENGINE_H

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
    QVariant operator() (const QByteArray& data);
private:
    bool get_process_func();
    QVariant to_variant(int n);
    std::string get_error_msg();

    lua_State* _lua = nullptr;
};

} // namespace Das

#endif // DAS_LUA_ENGINE_H
