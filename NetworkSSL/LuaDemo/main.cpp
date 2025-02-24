#include <iostream>
#include "../../include/lua/lua.hpp"
#include <Windows.h>     // MessageBoxA, etc.

#pragma comment(lib, "lua.lib")

//------------------------------
// 1) C++ �������� Lua �ܵ��� MessageBoxA
//    show_message_box("Text", "Title")
//------------------------------
static int l_show_message_box(lua_State* L) {
    // 1) ��ȡ����
    //    - ��һ����������Ϣ�ı� (const char*), 
    //    - �ڶ������������� (const char*).
    // ����ű������ڶ��������������Ĭ��ֵ
    const char* text = luaL_checkstring(L, 1);
    const char* title = luaL_optstring(L, 2, "Message");

    // 2) ���� MessageBoxA
    //    - ������HWND=0, LPCSTR= text, LPCSTR= title, UINT=0 (MB_OK)
    int result = MessageBoxA(NULL, text, title, MB_OK);

    // 3) �� MessageBoxA �ķ���ֵѹ�� Lua ջ
    lua_pushinteger(L, result);
    return 1; // ����ֵ����=1
}

//------------------------------
// main ������Ƕ�� Lua, ע�� show_message_box, ִ��ʾ���ű�
//------------------------------
int main() {
    //// 1) ���� Lua ״̬��
    //lua_State* L = luaL_newstate();
    //luaL_openlibs(L);

    //// 2) ע�� C++ ���� show_message_box �� Lua
    ////    - ������ Lua �ű�������� show_message_box(...)
    //lua_register(L, "show_message_box", l_show_message_box);

    //// 3) ׼��һ���򵥵�ʾ�� Lua �ű�
    //const char* luaScript = R"(
    //    -- Lua �ű������� C++ ע��� show_message_box
    //    local ret = show_message_box("Hello from Lua!", "Lua MessageBox")
    //    print("MessageBox returned: ".. ret)
    //)";

    //// 4) ִ�нű�
    //if (luaL_dostring(L, luaScript) != LUA_OK) {
    //    std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    //}

    //// 5) �ر� Lua
    //lua_close(L);
    //return 0;

    // 1) ���� Lua ״̬��
    lua_State* L = luaL_newstate();
    // 2) ���� Lua ��׼��
    luaL_openlibs(L);

    // 3) ���� Lua �ű�Ƭ�� (ֱ����Ƕ�� C++ �ַ���)
    const char* luaScript = R"(
        function getcfg()
            return 2
        end
    )";

    // 4) ִ������ű������غ��� getcfg ��ȫ��
    if (luaL_dostring(L, luaScript) != LUA_OK) {
        // ����ű��д���
        std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return 1;
    }

    // 5) �� C++ �е��� getcfg() ����
    //    (a) ��ȫ�ֵ� getcfg ѹ��ջ��
    lua_getglobal(L, "getcfg");

    //    (b) ȷ������һ������
    if (!lua_isfunction(L, -1)) {
        std::cerr << "'getcfg' is not a function." << std::endl;
        lua_close(L);
        return 1;
    }

    //    (c) ����������� (����=0, ����ֵ=1)
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        // ������ñ���
        std::cerr << "Error calling 'getcfg': " << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return 1;
    }

    //    (d) ��ȡ����ֵ
    //        ���ﺯ�������� 1 (һ�� number)
    int cfgValue = 0;
    if (lua_isnumber(L, -1)) {
        cfgValue = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1); // ��������ֵ

    // 6) ��ӡ���
    std::cout << "getcfg() returned: " << cfgValue << std::endl;

    // 7) �ر� Lua
    lua_close(L);
    return 0;

}
