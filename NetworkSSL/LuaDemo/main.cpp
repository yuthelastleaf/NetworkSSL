#include <iostream>
#include "../../include/lua/lua.hpp"
#include <Windows.h>     // MessageBoxA, etc.

#pragma comment(lib, "lua.lib")

//------------------------------
// 1) C++ 函数：让 Lua 能调用 MessageBoxA
//    show_message_box("Text", "Title")
//------------------------------
static int l_show_message_box(lua_State* L) {
    // 1) 获取参数
    //    - 第一个参数：消息文本 (const char*), 
    //    - 第二个参数：标题 (const char*).
    // 如果脚本不传第二个参数，可设个默认值
    const char* text = luaL_checkstring(L, 1);
    const char* title = luaL_optstring(L, 2, "Message");

    // 2) 调用 MessageBoxA
    //    - 参数：HWND=0, LPCSTR= text, LPCSTR= title, UINT=0 (MB_OK)
    int result = MessageBoxA(NULL, text, title, MB_OK);

    // 3) 将 MessageBoxA 的返回值压到 Lua 栈
    lua_pushinteger(L, result);
    return 1; // 返回值数量=1
}

//------------------------------
// main 函数：嵌入 Lua, 注册 show_message_box, 执行示例脚本
//------------------------------
int main() {
    // 1) 创建 Lua 状态机
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // 2) 注册 C++ 函数 show_message_box 到 Lua
    //    - 这样在 Lua 脚本里可以用 show_message_box(...)
    lua_register(L, "show_message_box", l_show_message_box);

    // 3) 准备一个简单的示例 Lua 脚本
    const char* luaScript = R"(
        -- Lua 脚本：调用 C++ 注册的 show_message_box
        local ret = show_message_box("Hello from Lua!", "Lua MessageBox")
        print("MessageBox returned: ".. ret)
    )";

    // 4) 执行脚本
    if (luaL_dostring(L, luaScript) != LUA_OK) {
        std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    }

    // 5) 关闭 Lua
    lua_close(L);
    return 0;
}
