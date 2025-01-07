#include "LuaFunc.h"

#include <atlstr.h>
#include <TlHelp32.h>

#include <fstream>
#include <taskschd.h>
#include <comdef.h>
#include <codecvt>

#include "../../include/StringHandler/StringHandler.h"


bool CreateDirectoriesRecursively(const std::string& dirPath);


// 方法：将 std::string 转换为 std::wstring，再转换回 std::string
std::string ConvertUtf8ToUtf8(std::string input) {
    // 1. 将 std::string 转换为 std::wstring (UTF-8 -> wide string)
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide_string = converter.from_bytes(input);

    // 2. 将 std::wstring 转换回 std::string (wide string -> UTF-8)
    char* str_utf8 = nullptr;
    CStringHandler::WChar2Ansi(wide_string.c_str(), str_utf8);
    std::string str_res = str_utf8;
    delete[] str_utf8;

    return str_res;
}

std::string GetCurrentProcessDirectory() {
    char path[MAX_PATH];

    // 获取当前进程的完整路径
    DWORD result = GetModuleFileNameA(NULL, path, MAX_PATH);

    if (result == 0) {
        OutputDebugStringA("Failed to get the current process path.");
        return "";
    }

    // 通过查找最后一个反斜杠来获得目录部分
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");

    if (pos != std::string::npos) {
        return fullPath.substr(0, pos);
    }

    return "";  // 如果没有找到反斜杠，返回空字符串
}

// ========== 6) kill_process ==========
// kill_process(pid) -> int(0/1)
// 通过进程ID打开并终止
static int l_sleep(lua_State* L) {
    DWORD stime = (DWORD)luaL_checkinteger(L, 1);

    Sleep(stime);

    return 1;
}

// ========== 6) 初始化中文环境 ==========
static int l_init_chs(lua_State* L) {
    CStringHandler::InitChinese();
    lua_pushinteger(L, 1);
    return 1;
}

// ========== 1) create_file ==========
// create_file(path, content) -> int(0=fail,1=ok)
// 在 path 创建文件，写入 content
static int l_create_file(lua_State* L) {
    // 1) 读取 Lua 参数
    const char* path = luaL_checkstring(L, 1);          // 必填
    const char* content = luaL_optstring(L, 2, "");     // 可选，默认空串

    // 2) 调用 Windows API 创建并写入
    HANDLE hFile = CreateFileA(
        path,
        GENERIC_WRITE,
        0,             // 不共享
        NULL,
        CREATE_ALWAYS, // 总是覆盖
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

    // 3) 返回结果
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

// Helper: 获取当前可执行文件路径
static std::string get_current_process_path() {
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        return std::string(buffer);
    }
    return "";
}

// C++ 实现的 Lua 方法：复制当前进程文件到目标目录
static int l_copy_current_exe(lua_State* L) {
    CStringHandler::InitChinese();


    // 获取 Lua 参数：目标目录
    const char* targetDir = luaL_checkstring(L, 1);  

    std::string conv_strdir = ConvertUtf8ToUtf8(targetDir);
    std::string currentExePath;
    if (lua_isstring(L, 2)) {
        currentExePath = conv_strdir;

        if (currentExePath.find(':') == std::string::npos) {
            currentExePath = GetCurrentProcessDirectory() + "\\" + currentExePath;
        }
        conv_strdir = ConvertUtf8ToUtf8(luaL_checkstring(L, 2));
    }
    else {
        // 获取当前可执行文件路径
        currentExePath = get_current_process_path();
        if (currentExePath.empty()) {
            lua_pushinteger(L, 0);
            return 1;
        }
    }
    bool flag = false;
    std::string str_target = conv_strdir;
    std::string targetPath;
    if (str_target.find('.') != std::string::npos) {
        flag = CreateDirectoriesRecursively(str_target.substr(0, str_target.find_last_of("\\")));
        targetPath = str_target;
    }
    else {
        flag = CreateDirectoriesRecursively(currentExePath);
        targetPath = conv_strdir + "\\" + currentExePath.substr(currentExePath.find_last_of("\\") + 1);
    }

    if (flag) {
        // 调用 CopyFileA 复制文件
        BOOL result = CopyFileA(currentExePath.c_str(), targetPath.c_str(), FALSE);
        if (!result) {
            lua_pushinteger(L, 0);
            return 1;
        }

        // 成功返回
        lua_pushinteger(L, 1);
    }
    else {
        lua_pushinteger(L, 0);
    }
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
// 简单示例：在文件末尾追加 text
static int l_modify_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const char* text = luaL_optstring(L, 2, "");

    // 打开文件，追加写
    HANDLE hFile = CreateFileA(
        path,
        FILE_APPEND_DATA, // 追加
        0,
        NULL,
        OPEN_EXISTING,   // 必须已存在
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
// start_process(exePath, cmdArgs) -> int(pid) 或 0 失败
// 简单示例：使用 CreateProcessA 启动进程
static int l_start_process(lua_State* L) {
    const char* exePath = luaL_checkstring(L, 1);
    // cmdArgs 可以为空
    const char* cmdArgs = luaL_optstring(L, 2, "");

    // 为 CreateProcess 准备命令行缓冲
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
        0,       // 无特殊标志
        NULL,
        NULL,
        &si,
        &pi
    );
    if (!ok) {
        lua_pushinteger(L, 0);
        return 1;
    }

    // 返回进程ID
    DWORD pid = pi.dwProcessId;

    // 关闭句柄（但进程继续运行）
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    lua_pushinteger(L, (lua_Integer)pid);
    return 1;
}

// ========== 6) kill_process ==========
// kill_process(pid) -> int(0/1)
// 通过进程ID打开并终止
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
// 注册表操作使用 WinAPI：
//   RegCreateKeyEx, RegSetValueEx, RegDeleteValue, RegDeleteKey, etc.
//
// 示例使用 HKCU 下操作
// 注意权限问题，若要写 HKLM 需管理员权限
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

// 3) modify_registry (同 add_registry)
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
// 返回 1 表示成功, 0 表示失败
static int l_set_registry_dword(lua_State* L) {
    // 1) 获取 Lua 参数
    const char* rootKeyStr = luaL_checkstring(L, 1);  // eg: "HKLM", "HKCU"
    const char* subKey = luaL_checkstring(L, 2);  // eg: "Software\\MyApp"
    const char* valueName = luaL_checkstring(L, 3);  // eg: "TestDword"

    // 第四个参数必须是整数
    lua_Integer val = luaL_checkinteger(L, 4);
    DWORD dwValue = (DWORD)val;

    // 2) 解析根键
    HKEY root = parse_root_key(rootKeyStr);

    // 3) 打开(或创建)子键
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
        // 写入失败, 返回 0
        lua_pushinteger(L, 0);
        return 1;
    }

    // 4) 写 DWORD
    ret = RegSetValueExA(hKey, valueName, 0, REG_DWORD, (BYTE*)&dwValue, sizeof(dwValue));
    RegCloseKey(hKey);

    // 5) 返回 1 或 0
    lua_pushinteger(L, (ret == ERROR_SUCCESS) ? 1 : 0);
    return 1;
}

// ========== 9) delete_registry ==========
// l_delete_registry("HKLM", "Software\\MyApp", "ValueName")
// delete_registry("Software\\MyApp", "ValueName") -> int(0/1)
// 仅删除值，不删除 key
static int l_delete_registry(lua_State* L) {
    const char* rootKeyStr = luaL_checkstring(L, 1);  // eg: "HKLM", "HKCU"
    const char* subKey = luaL_checkstring(L, 2);
    const char* valueName = luaL_checkstring(L, 3);

    // 2) 解析根键
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
    // 删除 value
    ret = RegDeleteValueA(hKey, valueName);
    RegCloseKey(hKey);

    lua_pushinteger(L, (ret == ERROR_SUCCESS) ? 1 : 0);
    return 1;
}

// kill_process_by_name("notepad.exe") -> int(关闭的进程数量)
static int l_kill_process_by_name(lua_State* L) {
    // 1) 获取 Lua 参数：进程名（可执行文件名，例如 "notepad.exe"）
    wchar_t* wproc_name = nullptr;
    do {
        const char* procName = luaL_checkstring(L, 1);
        
        if (!CStringHandler::Ansi2WChar(procName, wproc_name)) {
            break;
        }

        // 2) 创建快照，用于枚举系统中所有进程
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            lua_pushinteger(L, 0);
            break;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);

        // 3) 获取第一个进程
        if (!Process32First(snap, &pe32)) {
            CloseHandle(snap);
            lua_pushinteger(L, 0);
            break;
        }

        // 4) 遍历所有进程，记录关闭数量
        int count = 0;
        CString str_proc_name = wproc_name;
        do {
            // 将进程名与目标对比
            // 注意：szExeFile 为 ANSI 字符串
            if (!str_proc_name.CompareNoCase(pe32.szExeFile)) {
                // 匹配则尝试结束
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

        // 5) 返回关闭的进程数
        lua_pushinteger(L, count);
    } while (0);
    if (wproc_name) {
        delete[] wproc_name;
    }
    return 1;
}

// get_current_process_name() -> string
// 返回当前进程的可执行文件名 (如 "notepad.exe")，不含路径。
// 如果想返回完整路径，可直接把 path 整个返回。
static int l_get_current_process_name(lua_State* L) {
    char buf[MAX_PATH];
    // GetModuleFileNameA(NULL, ...) 可以获取当前可执行文件的完整路径。
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        // 出错或长度过长
        lua_pushnil(L);
        return 1;
    }

    // buf 里现在是完整路径，例如 "C:\MyFolder\my_program.exe"
    // 如果您只想返回文件名，可以提取最后一个斜杠后的部分：
    const char* fullPath = buf;
    const char* filename = fullPath + len; // 指向末尾

    // 向前查找最后一个反斜杠或斜杠
    while (filename > fullPath && *(filename - 1) != '\\' && *(filename - 1) != '/') {
        filename--;
    }
    // 此时 filename 指向真正的可执行文件名（例如 "my_program.exe"）

    // 将其压入 Lua 栈
    lua_pushstring(L, filename);
    return 1;
}


int l_register_driver(lua_State* L) {
    const char* driverPath = luaL_checkstring(L, 1);  // 驱动路径
    const char* serviceName = luaL_checkstring(L, 2);  // 服务名称

    std::string str_dv_path = driverPath;
    std::string str_ppath = str_dv_path;
    if (str_dv_path.find(':') == std::string::npos) {
        str_ppath = GetCurrentProcessDirectory() + "\\" + str_dv_path;
        std::ifstream file(str_ppath);
        if (!file.good()) {
            char systemDirectory[MAX_PATH];

            // 获取系统目录（如 C:\Windows\System32）
            if (!GetSystemDirectoryA(systemDirectory, MAX_PATH)) {
                OutputDebugStringA("System directory get failed .");
            }
            str_ppath = systemDirectory;
            str_ppath += "\\" + str_dv_path;
            std::ifstream refile(str_ppath);
            if (!refile.good()) {
                
                OutputDebugStringA("Failed to get driver path");
                str_ppath = str_dv_path;
            }
        }
    }

    // 通过 Lua 参数设置服务配置
    DWORD dwServiceType = SERVICE_KERNEL_DRIVER;  // 默认服务类型为驱动
    DWORD dwStartType = SERVICE_DEMAND_START;  // 默认启动类型为按需启动
    DWORD dwErrorControl = SERVICE_ERROR_NORMAL;  // 默认错误控制类型为正常
    const char* account = nullptr;
    const char* password = nullptr;

    // 读取 Lua 中的参数
    if (lua_isnumber(L, 3)) {
        dwServiceType = luaL_checkinteger(L, 3);
    }
    if (lua_isnumber(L, 4)) {
        dwStartType = luaL_checkinteger(L, 4);
    }
    if (lua_isnumber(L, 5)) {
        dwErrorControl = luaL_checkinteger(L, 5);
    }
    if (lua_isstring(L, 6)) {
        account = luaL_checkstring(L, 6);
    }
    if (lua_isstring(L, 7)) {
        password = luaL_checkstring(L, 7);
    }

    // 打开服务管理器
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        lua_pushstring(L, "Failed to open Service Control Manager");
        OutputDebugStringA("Failed to open Service Control Manager");
        return 1;
    }

    // 创建驱动服务
    SC_HANDLE hService = CreateServiceA(
        hSCManager,              // 服务控制管理器的句柄
        serviceName,             // 服务名称
        serviceName,             // 显示名称
        SERVICE_ALL_ACCESS,      // 需要的权限
        dwServiceType,           // 服务类型
        dwStartType,             // 启动类型
        dwErrorControl,          // 错误控制类型
        str_ppath.c_str(),              // 驱动路径
        NULL,                    // 不依赖其他服务
        NULL,                    // 不设置注册的服务标志
        NULL,                    // 不设置文件路径
        account,                 // 服务账户
        password);              // 服务密码

    if (!hService) {
        OutputDebugStringA("Failed to create service");
        lua_pushstring(L, "Failed to create service");
        CloseServiceHandle(hSCManager);
        return 1;
    }

    lua_pushstring(L, "Service created successfully");
    OutputDebugStringA("Service created successfully");
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 1;
}

int l_start_driver(lua_State* L) {
    const char* driverName = luaL_checkstring(L, 1);  // 获取驱动程序的名称

    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCM == NULL) {
        lua_pushboolean(L, false);
        return 1;
    }

    SC_HANDLE hService = OpenServiceA(hSCM, driverName, SERVICE_START);
    if (hService == NULL) {
        CloseServiceHandle(hSCM);
        lua_pushboolean(L, false);
        return 1;
    }

    // 启动驱动
    BOOL result = StartService(hService, 0, NULL);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);

    if (result == FALSE) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, true);
    return 1;
}

int l_stop_driver(lua_State* L) {
    const char* driverName = luaL_checkstring(L, 1);  // 获取驱动程序的名称

    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCM == NULL) {
        lua_pushboolean(L, false);
        return 1;
    }

    SC_HANDLE hService = OpenServiceA(hSCM, driverName, SERVICE_STOP);
    if (hService == NULL) {
        CloseServiceHandle(hSCM);
        lua_pushboolean(L, false);
        return 1;
    }

    SERVICE_STATUS status;
    BOOL result = ControlService(hService, SERVICE_CONTROL_STOP, &status);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);

    if (result == FALSE) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, true);
    return 1;
}

int l_uninstall_driver(lua_State* L) {
    const char* driverName = luaL_checkstring(L, 1);  // 获取驱动程序的名称

    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCM == NULL) {
        lua_pushboolean(L, false);
        return 1;
    }

    SC_HANDLE hService = OpenServiceA(hSCM, driverName, DELETE);
    if (hService == NULL) {
        CloseServiceHandle(hSCM);
        lua_pushboolean(L, false);
        return 1;
    }

    // 删除服务
    BOOL result = DeleteService(hService);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);

    if (result == FALSE) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, true);
    return 1;
}



// =============== Lua Runner ================ //

LuaRunner::LuaRunner() {
    lua_ = luaL_newstate();
    if (lua_) {
        luaL_openlibs(lua_);

        // 注册 C++ 函数到 Lua
        lua_register(lua_, "winsleep", l_sleep);
        lua_register(lua_, "create_file", l_create_file);
        lua_register(lua_, "move_file", l_move_file);
        lua_register(lua_, "delete_file", l_delete_file);
        lua_register(lua_, "modify_file", l_modify_file);
        lua_register(lua_, "copy_current_exe", l_copy_current_exe);
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

        lua_register(lua_, "register_driver", l_register_driver);
        lua_register(lua_, "start_driver", l_start_driver);
        lua_register(lua_, "stop_driver", l_stop_driver);
        lua_register(lua_, "uninstall_driver", l_uninstall_driver);

        lua_register(lua_, "init_chs", l_init_chs);
    }
}


// 提供的辅助函数

bool CreateDirectoriesRecursively(const std::string& dirPath) {
    DWORD dwAttrib = GetFileAttributesA(dirPath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        // 目录不存在，递归创建
        size_t pos = 0;
        while ((pos = dirPath.find_first_of("\\", pos)) != std::string::npos) {
            std::string subdir = dirPath.substr(0, pos);
            if (GetFileAttributesA(subdir.c_str()) == INVALID_FILE_ATTRIBUTES) {
                if (!CreateDirectoryA(subdir.c_str(), NULL)) {
                    std::string str_error = "Failed to create directory: ";
                    OutputDebugStringA((str_error + subdir).c_str());
                    return false;
                }
            }
            pos++;
        }
        if (!CreateDirectoryA(dirPath.c_str(), NULL)) {
            std::string str_error = "Failed to create directory: ";
            OutputDebugStringA((str_error + dirPath).c_str());
            return false;
        }
    }
    return true;  // 目录已经存在
}

// 新修改计划任务的注册，提供注册为system权限的配置，因出现为服务器提供升级，但是服务器一般只运行服务程序导致系统无法及时更新病毒库等内容
// 新增参数提供配置，兼容原有的使用并新增
// 配置 SYSTEM 为 SYSTEM权限，users为当前用户权限，一般是这样，并通过这种方式，在注册为system权限时，放弃活动桌面升级判断
static BOOL RegisterScheduledTaskInVistaOrLater(LPCTSTR lpTaskName, LPCTSTR lpExecutablePath, TASK_TRIGGER_TYPE2 TriggerType, int nDelayMinutes, BOOL bRemove = FALSE, CString user = L"Users")
{
	//  ------------------------------------------------------
	//  Initialize COM.
	WCHAR buffer[MAX_PATH];
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (FAILED(hr))
	{
		wsprintf(buffer, L"CoInitializeEx failed: %x", hr);
		::OutputDebugString(buffer);
		return FALSE;
	}

	//  Set general COM security levels.
	hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		0,
		NULL);

	if (FAILED(hr))
	{
		wsprintf(buffer, L"nCoInitializeSecurity failed: %x", hr);
		::OutputDebugString(buffer);
		CoUninitialize();
		return FALSE;
	}


	//  ------------------------------------------------------
	//  Create an instance of the Task Service. 
	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void**)&pService);
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Failed to create an instance of ITaskService: %x", hr);
		::OutputDebugString(buffer);
		CoUninitialize();
		return FALSE;
	}

	//  Connect to the task service.
	hr = pService->Connect(_variant_t(), _variant_t(),
		_variant_t(), _variant_t());
	if (FAILED(hr))
	{
		wsprintf(buffer, L"ITaskService::Connect failed: %x", hr);
		::OutputDebugString(buffer);
		pService->Release();
		CoUninitialize();
		return FALSE;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.  This folder will hold the
	//  new task that is registered.
	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot get Root Folder pointer: %x", hr);
		::OutputDebugString(buffer);
		pService->Release();
		CoUninitialize();
		return FALSE;
	}

	//  If the same task exists, remove it.
	pRootFolder->DeleteTask(_bstr_t(lpTaskName), 0);

	//如果是卸载模式，则返回
	if (bRemove)
	{
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return TRUE;
	}

	//  Create the task builder object to create the task.
	ITaskDefinition* pTask = NULL;
	hr = pService->NewTask(0, &pTask);

	pService->Release();  // COM clean up.  Pointer is no longer used.
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Failed to create a task definition: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		CoUninitialize();
		return FALSE;
	}

	//  ------------------------------------------------------
	//  Get the registration info for setting the identification.
	IRegistrationInfo* pRegInfo = NULL;
	hr = pTask->get_RegistrationInfo(&pRegInfo);
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot get identification pointer: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	DWORD buflen = MAX_PATH;
	GetUserNameW(buffer, &buflen);

	hr = pRegInfo->put_Author(buffer);
	pRegInfo->Release();
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot put identification info: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}


	//  ------------------------------------------------------
	//  Create the settings for the task
	ITaskSettings* pSettings = NULL;
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot get settings pointer: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	//  Set setting values for the task. 
	hr = pSettings->put_MultipleInstances(TASK_INSTANCES_PARALLEL);
	hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
	hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
	hr = pSettings->put_ExecutionTimeLimit(_bstr_t("PT0S"));

	IIdleSettings* pIdleSetting;
	hr = pSettings->get_IdleSettings(&pIdleSetting);
	pIdleSetting->put_IdleDuration(NULL);
	pIdleSetting->put_WaitTimeout(NULL);
	pIdleSetting->Release();

	pSettings->Release();
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot put setting info: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	//  ------------------------------------------------------
	//  Get the trigger collection to insert the logon trigger.
	ITriggerCollection* pTriggerCollection = NULL;
	hr = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot get trigger collection: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	//  Add the logon trigger to the task.
	ITrigger* pTrigger = NULL;
	hr = pTriggerCollection->Create(TriggerType, &pTrigger);
	pTriggerCollection->Release();
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot create the trigger: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	if (nDelayMinutes != 0)
	{
		if (TriggerType == TASK_TRIGGER_LOGON)
		{
			ILogonTrigger* pLogonTrigger = NULL;
			hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
			if (FAILED(hr))
			{
				wsprintf(buffer, L"Cannot get the logon trigger: %x", hr);
				::OutputDebugString(buffer);
				pRootFolder->Release();
				pTask->Release();
				CoUninitialize();
				return FALSE;
			}

			wsprintf(buffer, L"PT%dM", nDelayMinutes);
			hr = pLogonTrigger->put_Delay(_bstr_t(buffer));
			if (FAILED(hr))
			{
				wsprintf(buffer, L"put execute delay failed : %x", hr);
				::OutputDebugString(buffer);
				pRootFolder->Release();
				pTask->Release();
				CoUninitialize();
				return FALSE;
			}
		}
		else if (TriggerType == TASK_TRIGGER_BOOT)
		{
			IBootTrigger* pBootTrigger = NULL;
			hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pBootTrigger);
			if (FAILED(hr))
			{
				wsprintf(buffer, L"Cannot get the boot trigger: %x", hr);
				::OutputDebugString(buffer);
				pRootFolder->Release();
				pTask->Release();
				CoUninitialize();
				return FALSE;
			}

			wsprintf(buffer, L"PT%dM", nDelayMinutes);
			hr = pBootTrigger->put_Delay(_bstr_t(buffer));
			if (FAILED(hr))
			{
				wsprintf(buffer, L"put execute delay failed : %x", hr);
				::OutputDebugString(buffer);
				pRootFolder->Release();
				pTask->Release();
				CoUninitialize();
				return FALSE;
			}
		}
		else if (TriggerType == TASK_TRIGGER_REGISTRATION)
		{
			IRegistrationTrigger* pRegistrationTrigger = NULL;
			hr = pTrigger->QueryInterface(IID_IRegistrationTrigger, (void**)&pRegistrationTrigger);
			if (FAILED(hr))
			{
				wsprintf(buffer, L"Cannot get the registration trigger: %x", hr);
				::OutputDebugString(buffer);
				pRootFolder->Release();
				pTask->Release();
				CoUninitialize();
				return FALSE;
			}

			wsprintf(buffer, L"PT%dM", nDelayMinutes);
			hr = pRegistrationTrigger->put_Delay(_bstr_t(buffer));
			if (FAILED(hr))
			{
				wsprintf(buffer, L"put execute delay failed : %x", hr);
				::OutputDebugString(buffer);
				pRootFolder->Release();
				pTask->Release();
				CoUninitialize();
				return FALSE;
			}
		}
	}

	//  ------------------------------------------------------
	//  Add an Action to the task. This task will execute notepad.exe.     
	IActionCollection* pActionCollection = NULL;

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot get Task collection pointer: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	//  Create the action, specifying that it is an executable action.
	IAction* pAction = NULL;
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollection->Release();
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot create the action: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	IExecAction* pExecAction = NULL;
	//  QI for the executable task pointer.
	hr = pAction->QueryInterface(
		IID_IExecAction, (void**)&pExecAction);
	pAction->Release();
	if (FAILED(hr))
	{
		wsprintf(buffer, L"QueryInterface call failed for IExecAction: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	//  Set the path of the executable to notepad.exe.
	hr = pExecAction->put_Path(_bstr_t(lpExecutablePath));
	pExecAction->Release();
	if (FAILED(hr))
	{
		wsprintf(buffer, L"Cannot set path of executable: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	IPrincipal* pPrincipal = NULL;
	hr = pTask->get_Principal(&pPrincipal);
	pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

	//  ------------------------------------------------------
	//  Save the task in the root folder.
	IRegisteredTask* pRegisteredTask = NULL;
	//修改计划任务安全选项的用户标识为Users
	//解决域环境下 升级程序无法启动的问题
	//还需要为TDClientUpdate提升权限
	hr = pRootFolder->RegisterTaskDefinition(
		_bstr_t(lpTaskName),
		pTask,
		TASK_CREATE_OR_UPDATE,
		_variant_t(user),
		_variant_t(),
		TASK_LOGON_GROUP,
		_variant_t(L""),
		&pRegisteredTask);

	BSTR xml;
	pTask->get_XmlText(&xml);

	if (FAILED(hr))
	{
		wsprintf(buffer, L"Error saving the Task: %x", hr);
		::OutputDebugString(buffer);
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return FALSE;
	}

	wsprintf(buffer, L"Success! Task successfully registered. ");
	::OutputDebugString(buffer);

	// Clean up
	pRootFolder->Release();
	pTask->Release();

	//pRegisteredTask->Release();
	CoUninitialize();
	return TRUE;
}
