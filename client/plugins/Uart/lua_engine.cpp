#include <lua.hpp>

#include <Helpz/zfile.h>

#include "config.h"
#include "lua_engine.h"

namespace Das {

using namespace std;

/*static void dumpstack (lua_State *L)
{
    int top=lua_gettop(L);
    for (int i=1; i <= top; i++) {
        printf("%d\t%s\t", i, luaL_typename(L,i));
        switch (lua_type(L, i)) {
        case LUA_TNUMBER:
            printf("%g\n",lua_tonumber(L,i));
            break;
        case LUA_TSTRING:
            printf("%s\n",lua_tostring(L,i));
            break;
        case LUA_TBOOLEAN:
            printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
            break;
        case LUA_TNIL:
            printf("%s\n", "nil");
            break;
        default:
            printf("%p\n",lua_topointer(L,i));
            break;
        }
    }
}*/

Lua_Engine::~Lua_Engine()
{
    stop();
}

void Lua_Engine::init()
{
    if (_lua)
        stop();

    const Uart::Config& conf = Uart::Config::instance();
    if (conf._lua_script_file.empty())
        return;

    if (!Helpz::File::exist(conf._lua_script_file))
        throw runtime_error("Lua: script file doesn't exist: " + conf._lua_script_file);

    _lua = luaL_newstate();
    if (conf._lua_use_libs)
        luaL_openlibs(_lua);

    int res = luaL_dofile(_lua, conf._lua_script_file.c_str());
    if (res != LUA_OK)
    {
        string err_text = get_error_msg();
        stop();
        throw runtime_error(err_text);
    }
}

void Lua_Engine::stop()
{
    if (_lua)
    {
        lua_close(_lua);
        _lua = nullptr;
    }
}

Lua_Engine::operator bool() const { return _lua; }

QVariant Lua_Engine::operator()(const QByteArray &send_data, const QByteArray &recv_data,
                                const string &func_name, const QVariant &arg)
{
//    lua_settop(_lua, 0);
    if (!_lua || !get_process_func(func_name))
        return recv_data;

    push_bytearray(send_data);
    push_bytearray(recv_data);
    push_variant(arg);

    if (lua_pcall(_lua, 3, 2, 0) != LUA_OK)
        throw runtime_error(get_error_msg());

    QVariant val;

    bool ok = lua_toboolean(_lua, -2);
    if (ok)
        val = to_variant(-1);

    lua_pop(_lua, 2);
    return val;
}

bool Lua_Engine::get_process_func(const string &func_name)
{
    lua_getglobal(_lua, func_name.c_str());
    if (lua_isfunction(_lua, -1))
        return true;

    lua_pop(_lua, 1);
    return false;
}

void Lua_Engine::push_bytearray(const QByteArray &data)
{
    lua_newtable(_lua);

    for (int i = 0; i < data.size(); ++i)
    {
        // lua: #data - return last valid key. #data == data.size() - 1
        lua_pushnumber(_lua, i + 1); // note, you can replace this with a string if you want to access table cells by 'key'
        lua_pushinteger(_lua, static_cast<uint8_t>(data.at(i)));
        lua_settable(_lua, -3); // insert the new cell (and pop index/value off stack)
    }

//    lua_pushliteral(_lua, "n");
//    lua_pushnumber(_lua, recv_data.size()); // number of cells
//    lua_rawset(_lua, -3);
}

void Lua_Engine::push_variant(const QVariant &data)
{
    switch (data.type()) {

    case QVariant::Invalid:
        lua_pushnil(_lua);
        break;

    case QVariant::Bool:
        lua_pushboolean(_lua, data.toBool());
        break;

    case QVariant::Double:
        lua_pushnumber(_lua, data.toDouble());
        break;

    case QVariant::Int:
    case QVariant::UInt:
        lua_pushinteger(_lua, data.toInt());
        break;

    case QVariant::String:
    case QVariant::ByteArray:
    default:
        lua_pushstring(_lua, data.toString().toLocal8Bit().constData());
        break;
    }
}

QVariant Lua_Engine::to_variant(int n)
{
    switch (lua_type(_lua, n))
    {
    case LUA_TNUMBER:
        if (lua_isinteger(_lua, n))
            return lua_tointeger(_lua, n);
        else
            return lua_tonumber(_lua, n);

    case LUA_TSTRING:
        return QString(lua_tostring(_lua, n));

    case LUA_TBOOLEAN:
        return lua_toboolean(_lua, n) ? true : false;

    case LUA_TTABLE:
    {
        QVariantMap val;
        --n;
        lua_pushnil(_lua);
        while (lua_next(_lua, -2) != 0)
        {
            val.insert(to_variant(-2).toString(), to_variant(-1));
            lua_pop(_lua, 1);
        }
        return val;
    }
    default: break;
    }
    return {};
}

string Lua_Engine::get_error_msg()
{
    string err_text = string("Lua: ") + lua_tostring(_lua, -1);
    lua_pop(_lua, 1);
    return err_text;
}

} // namespace Das
