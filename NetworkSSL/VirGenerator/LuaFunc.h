#pragma once

#include "../../include/lua/lua.hpp"
#include <Windows.h>

class LuaRunner
{
public:
    LuaRunner();

    ~LuaRunner() {
        lua_close(lua_);
    }

    bool run_lua(const char* lua_data) {
        int flag = luaL_dostring(lua_, lua_data);
        if (flag != LUA_OK) {
            OutputDebugString(L"lua script run failed .");
        }
        return !flag;
    }

    int get_lua_cfg(const char* lua_data) {
        int cfg = 0;
        do {
            // 4) 执行这个脚本，加载函数 getcfg 到全局
            if (luaL_dostring(lua_, lua_data) != LUA_OK) {
                // 如果脚本有错误
                OutputDebugString(L"lua script getcfg run failed .");
                break;
            }

            // 5) 在 C++ 中调用 getcfg() 函数
            //    (a) 将全局的 getcfg 压到栈顶
            lua_getglobal(lua_, "getcfg");

            //    (b) 确保它是一个函数
            if (!lua_isfunction(lua_, -1)) {
                OutputDebugString(L"'getcfg' is not a function.");
                break;
            }

            //    (c) 调用这个函数 (参数=0, 返回值=1)
            if (lua_pcall(lua_, 0, 1, 0) != LUA_OK) {
                // 如果调用报错
                OutputDebugString(L"Error calling 'getcfg': ");
                break;
            }

            //    (d) 获取返回值
            //        这里函数返回了 1 (一个 number)
            if (lua_isnumber(lua_, -1)) {
                cfg = (int)lua_tointeger(lua_, -1);
            }
            lua_pop(lua_, 1); // 弹出返回值
        } while (0);
        return cfg;
    }

private:
    lua_State* lua_;
};
