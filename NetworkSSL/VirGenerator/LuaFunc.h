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
            // 4) ִ������ű������غ��� getcfg ��ȫ��
            if (luaL_dostring(lua_, lua_data) != LUA_OK) {
                // ����ű��д���
                OutputDebugString(L"lua script getcfg run failed .");
                break;
            }

            // 5) �� C++ �е��� getcfg() ����
            //    (a) ��ȫ�ֵ� getcfg ѹ��ջ��
            lua_getglobal(lua_, "getcfg");

            //    (b) ȷ������һ������
            if (!lua_isfunction(lua_, -1)) {
                OutputDebugString(L"'getcfg' is not a function.");
                break;
            }

            //    (c) ����������� (����=0, ����ֵ=1)
            if (lua_pcall(lua_, 0, 1, 0) != LUA_OK) {
                // ������ñ���
                OutputDebugString(L"Error calling 'getcfg': ");
                break;
            }

            //    (d) ��ȡ����ֵ
            //        ���ﺯ�������� 1 (һ�� number)
            if (lua_isnumber(lua_, -1)) {
                cfg = (int)lua_tointeger(lua_, -1);
            }
            lua_pop(lua_, 1); // ��������ֵ
        } while (0);
        return cfg;
    }

private:
    lua_State* lua_;
};
