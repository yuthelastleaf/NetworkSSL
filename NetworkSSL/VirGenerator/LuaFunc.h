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

        // ��ȡ�ļ���С
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        // ���仺����
        char* buffer = new char[fileSize + 1];  // ��һ���ֽ����ڴ洢 '\0' �ַ�

        // ��ȡ�ļ����ݵ�������
        size_t bytesRead = fread_s(buffer, fileSize + 1, sizeof(char), fileSize, file);
        if (bytesRead == 0) {
            OutputDebugStringA("Error reading file!");
            delete[] buffer;
            fclose(file);
            return -1;
        }
        buffer[fileSize] = '\0';  // ȷ���ַ�����ֹ

        run_lua(buffer);

        // �ر��ļ����ͷ��ڴ�
        fclose(file);
        delete[] buffer;
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
