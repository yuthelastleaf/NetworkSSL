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
    // 1) ���� Lua ״̬��
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // 2) ע�� C++ ���� show_message_box �� Lua
    //    - ������ Lua �ű�������� show_message_box(...)
    lua_register(L, "show_message_box", l_show_message_box);

    // 3) ׼��һ���򵥵�ʾ�� Lua �ű�
    const char* luaScript = R"(
        -- Lua �ű������� C++ ע��� show_message_box
        local ret = show_message_box("Hello from Lua!", "Lua MessageBox")
        print("MessageBox returned: ".. ret)
    )";

    // 4) ִ�нű�
    if (luaL_dostring(L, luaScript) != LUA_OK) {
        std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    }

    // 5) �ر� Lua
    lua_close(L);
    return 0;
}
