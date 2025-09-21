#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

#include <locale.h>

class DLLInjector {
private:
    HANDLE hProcess;
    DWORD targetPID;

public:
    DLLInjector() : hProcess(NULL), targetPID(0) {}

    ~DLLInjector() {
        if (hProcess) {
            CloseHandle(hProcess);
        }
    }

    // 根据进程名查找PID
    DWORD FindProcessByName(const std::wstring& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (processName == pe32.szExeFile) {
                    CloseHandle(hSnapshot);
                    return pe32.th32ProcessID;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
        return 0;
    }

    // 打开目标进程
    bool OpenTargetProcess(DWORD pid) {
        targetPID = pid;
        hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
            FALSE, pid);

        if (!hProcess) {
            std::wcout << L"[-] 无法打开进程 PID: " << pid
                << L" 错误码: " << GetLastError() << std::endl;
            return false;
        }

        std::wcout << L"[+] 成功打开目标进程 PID: " << pid << std::endl;
        return true;
    }

    // 方法1：传统LoadLibrary注入
    bool InjectDLL_LoadLibrary(const std::wstring& dllPath) {
        if (!hProcess) {
            std::wcout << L"[-] 进程句柄无效" << std::endl;
            return false;
        }

        // 检查DLL文件是否存在
        if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::wcout << L"[-] DLL文件不存在: " << dllPath << std::endl;
            return false;
        }

        // 计算路径长度
        SIZE_T pathSize = (dllPath.length() + 1) * sizeof(wchar_t);

        // 在目标进程中分配内存
        LPVOID remotePath = VirtualAllocEx(hProcess, NULL, pathSize,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath) {
            std::wcout << L"[-] 无法在目标进程中分配内存，错误码: " << GetLastError() << std::endl;
            return false;
        }

        std::wcout << L"[+] 在目标进程中分配内存: 0x" << std::hex << remotePath << std::endl;

        // 写入DLL路径到目标进程
        if (!WriteProcessMemory(hProcess, remotePath, dllPath.c_str(), pathSize, NULL)) {
            std::wcout << L"[-] 无法写入DLL路径到目标进程，错误码: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] 成功写入DLL路径到目标进程" << std::endl;

        // 获取LoadLibraryW函数地址
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!hKernel32) {
            std::wcout << L"[-] 无法获取kernel32.dll句柄" << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        LPTHREAD_START_ROUTINE loadLibraryAddr =
            (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
        if (!loadLibraryAddr) {
            std::wcout << L"[-] 无法获取LoadLibraryW地址" << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] LoadLibraryW地址: 0x" << std::hex << loadLibraryAddr << std::endl;

        // 创建远程线程执行LoadLibraryW
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryAddr,
            remotePath, 0, NULL);
        if (!hThread) {
            std::wcout << L"[-] 无法创建远程线程，错误码: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] 成功创建远程线程，等待执行完成..." << std::endl;

        // 等待线程执行完成
        WaitForSingleObject(hThread, INFINITE);

        // 获取线程退出码（LoadLibrary的返回值，即DLL的基地址）
        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);

        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);

        if (exitCode == 0) {
            std::wcout << L"[-] DLL注入失败，LoadLibrary返回NULL" << std::endl;
            return false;
        }

        std::wcout << L"[+] DLL注入成功！DLL基地址: 0x" << std::hex << exitCode << std::endl;
        return true;
    }

    // 方法2：Shellcode注入方式
    bool InjectDLL_Shellcode(const std::wstring& dllPath) {
        if (!hProcess) {
            std::wcout << L"[-] 进程句柄无效" << std::endl;
            return false;
        }

        // 构造shellcode来调用LoadLibraryW
        // 这种方式可以更好地控制执行流程和错误处理

        // 获取必要的API地址
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        LPVOID loadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
        LPVOID getLastError = GetProcAddress(hKernel32, "GetLastError");

        if (!loadLibraryW || !getLastError) {
            std::wcout << L"[-] 无法获取必要的API地址" << std::endl;
            return false;
        }

        // 分配内存存储DLL路径
        SIZE_T pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
        LPVOID remotePath = VirtualAllocEx(hProcess, NULL, pathSize,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath) {
            return false;
        }

        WriteProcessMemory(hProcess, remotePath, dllPath.c_str(), pathSize, NULL);

        // 分配shellcode内存
        SIZE_T shellcodeSize = 1024;
        LPVOID remoteShellcode = VirtualAllocEx(hProcess, NULL, shellcodeSize,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!remoteShellcode) {
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        // 构造shellcode（x64）
        std::vector<BYTE> shellcode;

#ifdef _WIN64
        // x64 shellcode
        shellcode = {
            0x48, 0x83, 0xEC, 0x28,                                     // sub rsp, 0x28
            0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rcx, dllPath
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, LoadLibraryW
            0xFF, 0xD0,                                                 // call rax
            0x48, 0x83, 0xC4, 0x28,                                     // add rsp, 0x28
            0xC3                                                        // ret
        };

        // 填入实际地址
        *(UINT64*)(&shellcode[6]) = (UINT64)remotePath;
        *(UINT64*)(&shellcode[16]) = (UINT64)loadLibraryW;
#else
        // x86 shellcode
        shellcode = {
            0x68, 0x00, 0x00, 0x00, 0x00,   // push dllPath
            0xB8, 0x00, 0x00, 0x00, 0x00,   // mov eax, LoadLibraryW
            0xFF, 0xD0,                     // call eax
            0xC3                            // ret
        };

        // 填入实际地址
        *(DWORD*)(&shellcode[1]) = (DWORD)remotePath;
        *(DWORD*)(&shellcode[6]) = (DWORD)loadLibraryW;
#endif

        // 写入shellcode
        if (!WriteProcessMemory(hProcess, remoteShellcode, shellcode.data(),
            shellcode.size(), NULL)) {
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            VirtualFreeEx(hProcess, remoteShellcode, 0, MEM_RELEASE);
            return false;
        }

        // 执行shellcode
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE)remoteShellcode,
            NULL, 0, NULL);
        if (!hThread) {
            std::wcout << L"[-] 无法创建远程线程执行shellcode，错误码: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            VirtualFreeEx(hProcess, remoteShellcode, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] 通过shellcode执行DLL注入..." << std::endl;

        WaitForSingleObject(hThread, INFINITE);

        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);

        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, remoteShellcode, 0, MEM_RELEASE);

        if (exitCode == 0) {
            std::wcout << L"[-] Shellcode注入失败" << std::endl;
            return false;
        }

        std::wcout << L"[+] Shellcode注入成功！DLL基地址: 0x" << std::hex << exitCode << std::endl;
        return true;
    }

    // 检查进程架构匹配
    bool CheckArchitecture() {
        if (!hProcess) return false;

        BOOL isWow64Process = FALSE;
        BOOL isWow64Current = FALSE;

        IsWow64Process(hProcess, &isWow64Process);
        IsWow64Process(GetCurrentProcess(), &isWow64Current);

        if (isWow64Process != isWow64Current) {
            std::wcout << L"[-] 警告：进程架构不匹配！" << std::endl;
            std::wcout << L"    目标进程 WOW64: " << (isWow64Process ? L"是" : L"否") << std::endl;
            std::wcout << L"    当前进程 WOW64: " << (isWow64Current ? L"是" : L"否") << std::endl;
            return false;
        }

        return true;
    }

    // 列出所有进程
    void ListProcesses() {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        std::wcout << L"\n=== 当前运行的进程 ===" << std::endl;
        std::wcout << L"PID\t进程名" << std::endl;
        std::wcout << L"---\t------" << std::endl;

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wcout << pe32.th32ProcessID << L"\t" << pe32.szExeFile << std::endl;
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
    }
};

// 使用示例
int wmain(int argc, wchar_t* argv[]) {
    setlocale(LC_ALL, "chs");

    std::wcout << L"=== DLL注入器 - CreateRemoteThread版本 ===" << std::endl;

    if (argc < 3) {
        std::wcout << L"用法: " << argv[0] << L" <进程名或PID> <DLL路径> [方法]" << std::endl;
        std::wcout << L"方法: 1=LoadLibrary(默认), 2=Shellcode" << std::endl;
        std::wcout << L"示例: " << argv[0] << L" notepad.exe C:\\MyDLL.dll 1" << std::endl;
        return 1;
    }

    DLLInjector injector;

    // 如果需要查看进程列表
    if (wcscmp(argv[1], L"list") == 0) {
        injector.ListProcesses();
        return 0;
    }

    // 确定目标进程PID
    DWORD targetPID = 0;
    if (iswdigit(argv[1][0])) {
        // 如果是数字，当作PID处理
        targetPID = _wtoi(argv[1]);
    }
    else {
        // 如果是字符串，当作进程名处理
        targetPID = injector.FindProcessByName(argv[1]);
        if (targetPID == 0) {
            std::wcout << L"[-] 未找到进程: " << argv[1] << std::endl;
            return 1;
        }
    }

    std::wcout << L"[+] 目标进程PID: " << targetPID << std::endl;

    // 打开目标进程
    if (!injector.OpenTargetProcess(targetPID)) {
        return 1;
    }

    // 检查架构匹配
    if (!injector.CheckArchitecture()) {
        std::wcout << L"[-] 架构不匹配，可能导致注入失败" << std::endl;
    }

    // 获取DLL路径
    std::wstring dllPath = argv[2];

    // 确定注入方法
    int method = (argc >= 4) ? _wtoi(argv[3]) : 1;

    bool success = false;
    switch (method) {
    case 1:
        std::wcout << L"[*] 使用LoadLibrary方法注入..." << std::endl;
        success = injector.InjectDLL_LoadLibrary(dllPath);
        break;
    case 2:
        std::wcout << L"[*] 使用Shellcode方法注入..." << std::endl;
        success = injector.InjectDLL_Shellcode(dllPath);
        break;
    default:
        std::wcout << L"[-] 不支持的注入方法: " << method << std::endl;
        return 1;
    }

    if (success) {
        std::wcout << L"[+] DLL注入完成！" << std::endl;
        return 0;
    }
    else {
        std::wcout << L"[-] DLL注入失败！" << std::endl;
        return 1;
    }
}