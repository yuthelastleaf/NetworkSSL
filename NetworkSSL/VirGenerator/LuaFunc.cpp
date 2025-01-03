#include "LuaFunc.h"
#include "../../include/StringHandler/StringHandler.h"
#include <atlstr.h>
#include <TlHelp32.h>
#include <string>

// ========== 1) create_file ==========
// create_file(path, content) -> int(0=fail,1=ok)
// �� path �����ļ���д�� content
static int l_create_file(lua_State* L) {
    // 1) ��ȡ Lua ����
    const char* path = luaL_checkstring(L, 1);          // ����
    const char* content = luaL_optstring(L, 2, "");     // ��ѡ��Ĭ�Ͽմ�

    // 2) ���� Windows API ������д��
    HANDLE hFile = CreateFileA(
        path,
        GENERIC_WRITE,
        0,             // ������
        NULL,
        CREATE_ALWAYS, // ���Ǹ���
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        lua_pushinteger(L, 0); // fail
        return 1;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, content, (DWORD)strlen(content), &written, NULL);
    CloseHandle(hFile);
    if (!ok) {
        lua_pushinteger(L, 0);
        return 1;
    }

    // 3) ���ؽ��
    lua_pushinteger(L, 1); // success
    return 1;
}

// ========== 2) move_file ==========
// move_file(oldPath, newPath) -> int(0/1)
static int l_move_file(lua_State* L) {
    const char* oldPath = luaL_checkstring(L, 1);
    const char* newPath = luaL_checkstring(L, 2);

    BOOL ok = MoveFileA(oldPath, newPath);
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// ========== 3) delete_file ==========
// delete_file(path) -> int(0/1)
static int l_delete_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    BOOL ok = DeleteFileA(path);
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// ========== 4) modify_file ==========
// modify_file(path, text) -> int(0/1)
// ��ʾ�������ļ�ĩβ׷�� text
static int l_modify_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const char* text = luaL_optstring(L, 2, "");

    // ���ļ���׷��д
    HANDLE hFile = CreateFileA(
        path,
        FILE_APPEND_DATA, // ׷��
        0,
        NULL,
        OPEN_EXISTING,   // �����Ѵ���
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        lua_pushinteger(L, 0);
        return 1;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, text, (DWORD)strlen(text), &written, NULL);
    CloseHandle(hFile);

    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

// ========== 5) start_process ==========
// start_process(exePath, cmdArgs) -> int(pid) �� 0 ʧ��
// ��ʾ����ʹ�� CreateProcessA ��������
static int l_start_process(lua_State* L) {
    const char* exePath = luaL_checkstring(L, 1);
    // cmdArgs ����Ϊ��
    const char* cmdArgs = luaL_optstring(L, 2, "");

    // Ϊ CreateProcess ׼�������л���
    std::string cmdLine = std::string(exePath) + " " + cmdArgs;

    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = { 0 };

    BOOL ok = CreateProcessA(
        exePath,
        (LPSTR)cmdLine.c_str(),
        NULL,
        NULL,
        FALSE,
        0,       // �������־
        NULL,
        NULL,
        &si,
        &pi
    );
    if (!ok) {
        lua_pushinteger(L, 0);
        return 1;
    }

    // ���ؽ���ID
    DWORD pid = pi.dwProcessId;

    // �رվ���������̼������У�
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    lua_pushinteger(L, (lua_Integer)pid);
    return 1;
}

// ========== 6) kill_process ==========
// kill_process(pid) -> int(0/1)
// ͨ������ID�򿪲���ֹ
static int l_kill_process(lua_State* L) {
    DWORD pid = (DWORD)luaL_checkinteger(L, 1);

    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) {
        lua_pushinteger(L, 0);
        return 1;
    }
    BOOL ok = TerminateProcess(hProc, 1);
    CloseHandle(hProc);
    lua_pushinteger(L, ok ? 1 : 0);
    return 1;
}

//------------------------------
// ע������ʹ�� WinAPI��
//   RegCreateKeyEx, RegSetValueEx, RegDeleteValue, RegDeleteKey, etc.
//
// ʾ��ʹ�� HKCU �²���
// ע��Ȩ�����⣬��Ҫд HKLM �����ԱȨ��
//------------------------------

// 1) parse_root_key
static HKEY parse_root_key(const std::string& rootKeyName) {
    if (rootKeyName == "HKLM")  return HKEY_LOCAL_MACHINE;
    if (rootKeyName == "HKCU")  return HKEY_CURRENT_USER;
    if (rootKeyName == "HKCR")  return HKEY_CLASSES_ROOT;
    if (rootKeyName == "HKU")   return HKEY_USERS;
    if (rootKeyName == "HKCC")  return HKEY_CURRENT_CONFIG;
    return HKEY_CURRENT_USER;
}

// 2) add_registry
static int l_add_registry(lua_State* L) {
    const char* rootKeyStr = luaL_checkstring(L, 1);
    const char* subKey = luaL_checkstring(L, 2);
    const char* valueName = luaL_checkstring(L, 3);

    bool isInt = lua_isinteger(L, 4);
    bool isString = lua_isstring(L, 4);

    DWORD dwType = 0;
    DWORD dwData = 0;
    std::string strData;

    if (isInt) {
        dwData = (DWORD)lua_tointeger(L, 4);
        dwType = REG_DWORD;
    }
    else if (isString) {
        const char* s = lua_tostring(L, 4);
        strData = (s ? s : "");
        dwType = REG_SZ;
    }
    else {
        // default: empty string
        strData = "";
        dwType = REG_SZ;
    }

    HKEY root = parse_root_key(rootKeyStr);

    HKEY hKey = NULL;
    LONG ret = RegCreateKeyExA(root, subKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (ret != ERROR_SUCCESS) {
        lua_pushinteger(L, 0);
        return 1;
    }

    if (dwType == REG_DWORD) {
        ret = RegSetValueExA(hKey, valueName, 0, REG_DWORD, (BYTE*)&dwData, sizeof(dwData));
    }
    else {
        ret = RegSetValueExA(hKey, valueName, 0, REG_SZ,
            (const BYTE*)strData.c_str(),
            (DWORD)strData.size() + 1);
    }
    RegCloseKey(hKey);

    lua_pushinteger(L, (ret == ERROR_SUCCESS) ? 1 : 0);
    return 1;
}

// 3) modify_registry (ͬ add_registry)
static int l_modify_registry(lua_State* L) {
    return l_add_registry(L);
}

// 4) get_registry_string
static int l_get_registry_string(lua_State* L) {
    const char* rootKeyStr = luaL_checkstring(L, 1);
    const char* subKey = luaL_checkstring(L, 2);
    const char* valueName = luaL_checkstring(L, 3);

    HKEY root = parse_root_key(rootKeyStr);

    HKEY hKey;
    LONG ret = RegOpenKeyExA(root, subKey, 0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS) {
        lua_pushnil(L);
        return 1;
    }

    DWORD dwType = 0;
    DWORD dwSize = 0;
    ret = RegQueryValueExA(hKey, valueName, NULL, &dwType, NULL, &dwSize);
    if (ret != ERROR_SUCCESS || dwType != REG_SZ) {
        RegCloseKey(hKey);
        lua_pushnil(L);
        return 1;
    }

    std::string strData;
    strData.resize(dwSize);
    ret = RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)strData.data(), &dwSize);
    RegCloseKey(hKey);

    if (ret == ERROR_SUCCESS) {
        lua_pushstring(L, strData.c_str());
    }
    else {
        lua_pushnil(L);
    }
    return 1;
}

// 5) get_registry_dword
static int l_get_registry_dword(lua_State* L) {
    const char* rootKeyStr = luaL_checkstring(L, 1);
    const char* subKey = luaL_checkstring(L, 2);
    const char* valueName = luaL_checkstring(L, 3);

    HKEY root = parse_root_key(rootKeyStr);

    HKEY hKey;
    LONG ret = RegOpenKeyExA(root, subKey, 0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS) {
        lua_pushnil(L);
        return 1;
    }

    DWORD dwType = 0;
    DWORD dwData = 0;
    DWORD dwSize = sizeof(DWORD);
    ret = RegQueryValueExA(hKey, valueName, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    RegCloseKey(hKey);

    if (ret == ERROR_SUCCESS && dwType == REG_DWORD) {
        lua_pushinteger(L, (lua_Integer)dwData);
    }
    else {
        lua_pushnil(L);
    }
    return 1;
}

// set_registry_dword("HKLM", "Software\\MyApp", "ValueName", 1234)
// ���� 1 ��ʾ�ɹ�, 0 ��ʾʧ��
static int l_set_registry_dword(lua_State* L) {
    // 1) ��ȡ Lua ����
    const char* rootKeyStr = luaL_checkstring(L, 1);  // eg: "HKLM", "HKCU"
    const char* subKey = luaL_checkstring(L, 2);  // eg: "Software\\MyApp"
    const char* valueName = luaL_checkstring(L, 3);  // eg: "TestDword"

    // ���ĸ���������������
    lua_Integer val = luaL_checkinteger(L, 4);
    DWORD dwValue = (DWORD)val;

    // 2) ��������
    HKEY root = parse_root_key(rootKeyStr);

    // 3) ��(�򴴽�)�Ӽ�
    HKEY hKey = NULL;
    LONG ret = RegCreateKeyExA(
        root,
        subKey,
        0,
        NULL,
        0,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );
    if (ret != ERROR_SUCCESS) {
        // д��ʧ��, ���� 0
        lua_pushinteger(L, 0);
        return 1;
    }

    // 4) д DWORD
    ret = RegSetValueExA(hKey, valueName, 0, REG_DWORD, (BYTE*)&dwValue, sizeof(dwValue));
    RegCloseKey(hKey);

    // 5) ���� 1 �� 0
    lua_pushinteger(L, (ret == ERROR_SUCCESS) ? 1 : 0);
    return 1;
}

// ========== 9) delete_registry ==========
// l_delete_registry("HKLM", "Software\\MyApp", "ValueName")
// delete_registry("Software\\MyApp", "ValueName") -> int(0/1)
// ��ɾ��ֵ����ɾ�� key
static int l_delete_registry(lua_State* L) {
    const char* rootKeyStr = luaL_checkstring(L, 1);  // eg: "HKLM", "HKCU"
    const char* subKey = luaL_checkstring(L, 2);
    const char* valueName = luaL_checkstring(L, 3);

    // 2) ��������
    HKEY root = parse_root_key(rootKeyStr);

    HKEY hKey;
    LONG ret = RegOpenKeyExA(
        root,
        subKey,
        0,
        KEY_WRITE,
        &hKey
    );
    if (ret != ERROR_SUCCESS) {
        lua_pushinteger(L, 0);
        return 1;
    }
    // ɾ�� value
    ret = RegDeleteValueA(hKey, valueName);
    RegCloseKey(hKey);

    lua_pushinteger(L, (ret == ERROR_SUCCESS) ? 1 : 0);
    return 1;
}

// kill_process_by_name("notepad.exe") -> int(�رյĽ�������)
static int l_kill_process_by_name(lua_State* L) {
    // 1) ��ȡ Lua ����������������ִ���ļ��������� "notepad.exe"��
    wchar_t* wproc_name = nullptr;
    do {
        const char* procName = luaL_checkstring(L, 1);
        
        if (!CStringHandler::Ansi2WChar(procName, wproc_name)) {
            break;
        }

        // 2) �������գ�����ö��ϵͳ�����н���
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            lua_pushinteger(L, 0);
            break;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);

        // 3) ��ȡ��һ������
        if (!Process32First(snap, &pe32)) {
            CloseHandle(snap);
            lua_pushinteger(L, 0);
            break;
        }

        // 4) �������н��̣���¼�ر�����
        int count = 0;
        CString str_proc_name = wproc_name;
        do {
            // ����������Ŀ��Ա�
            // ע�⣺szExeFile Ϊ ANSI �ַ���
            if (!str_proc_name.CompareNoCase(pe32.szExeFile)) {
                // ƥ�����Խ���
                DWORD pid = pe32.th32ProcessID;
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProc) {
                    if (TerminateProcess(hProc, 1)) {
                        count++;
                    }
                    CloseHandle(hProc);
                }
            }
        } while (Process32Next(snap, &pe32));

        CloseHandle(snap);

        // 5) ���عرյĽ�����
        lua_pushinteger(L, count);
    } while (0);
    if (wproc_name) {
        delete[] wproc_name;
    }
    return 1;
}

// get_current_process_name() -> string
// ���ص�ǰ���̵Ŀ�ִ���ļ��� (�� "notepad.exe")������·����
// ����뷵������·������ֱ�Ӱ� path �������ء�
static int l_get_current_process_name(lua_State* L) {
    char buf[MAX_PATH];
    // GetModuleFileNameA(NULL, ...) ���Ի�ȡ��ǰ��ִ���ļ�������·����
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        // ����򳤶ȹ���
        lua_pushnil(L);
        return 1;
    }

    // buf ������������·�������� "C:\MyFolder\my_program.exe"
    // �����ֻ�뷵���ļ�����������ȡ���һ��б�ܺ�Ĳ��֣�
    const char* fullPath = buf;
    const char* filename = fullPath + len; // ָ��ĩβ

    // ��ǰ�������һ����б�ܻ�б��
    while (filename > fullPath && *(filename - 1) != '\\' && *(filename - 1) != '/') {
        filename--;
    }
    // ��ʱ filename ָ�������Ŀ�ִ���ļ��������� "my_program.exe"��

    // ����ѹ�� Lua ջ
    lua_pushstring(L, filename);
    return 1;
}

// =============== Lua Runner ================ //

LuaRunner::LuaRunner() {
    lua_ = luaL_newstate();
    if (lua_) {
        luaL_openlibs(lua_);

        // ע�� C++ ������ Lua
        lua_register(lua_, "create_file", l_create_file);
        lua_register(lua_, "move_file", l_move_file);
        lua_register(lua_, "delete_file", l_delete_file);
        lua_register(lua_, "modify_file", l_modify_file);
        lua_register(lua_, "start_process", l_start_process);
        lua_register(lua_, "kill_process", l_kill_process);
        lua_register(lua_, "add_registry", l_add_registry);
        lua_register(lua_, "modify_registry", l_modify_registry);
        lua_register(lua_, "delete_registry", l_delete_registry);
        lua_register(lua_, "get_registry_string", l_get_registry_string);
        lua_register(lua_, "get_registry_dword", l_get_registry_dword);
        lua_register(lua_, "set_registry_dword", l_set_registry_dword);
        lua_register(lua_, "kill_process_by_name", l_kill_process_by_name);
        lua_register(lua_, "get_current_process_name", l_get_current_process_name);
    }
}


