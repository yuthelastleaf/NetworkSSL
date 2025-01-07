#pragma once

#include "../../include/lua/lua.hpp"
#include <Windows.h>

#include <string>

std::string GetCurrentProcessDirectory();

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

    bool run_lua_file(const char* lua_file) {
        FILE* file = NULL;

        std::string str_path = lua_file;
        if (str_path.find_first_of(':') == std::string::npos) {
            str_path = GetCurrentProcessDirectory() + "\\" + lua_file;
            fopen_s(&file, str_path.c_str(), "rb");
        }
        else {
            fopen_s(&file, lua_file, "rb");
        }

        if (file == nullptr) {
            OutputDebugStringA("Failed to open file!");
            return -1;
        }

        // 获取文件大小
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        // 分配缓冲区
        char* buffer = new char[fileSize + 1];  // 多一个字节用于存储 '\0' 字符

        // 读取文件数据到缓冲区
        size_t bytesRead = fread_s(buffer, fileSize + 1, sizeof(char), fileSize, file);
        if (bytesRead == 0) {
            OutputDebugStringA("Error reading file!");
            delete[] buffer;
            fclose(file);
            return -1;
        }
        buffer[fileSize] = '\0';  // 确保字符串终止

        run_lua(buffer);

        // 关闭文件并释放内存
        fclose(file);
        delete[] buffer;
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
