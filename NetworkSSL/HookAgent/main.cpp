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

    // ���ݽ���������PID
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

    // ��Ŀ�����
    bool OpenTargetProcess(DWORD pid) {
        targetPID = pid;
        hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
            FALSE, pid);

        if (!hProcess) {
            std::wcout << L"[-] �޷��򿪽��� PID: " << pid
                << L" ������: " << GetLastError() << std::endl;
            return false;
        }

        std::wcout << L"[+] �ɹ���Ŀ����� PID: " << pid << std::endl;
        return true;
    }

    // ����1����ͳLoadLibraryע��
    bool InjectDLL_LoadLibrary(const std::wstring& dllPath) {
        if (!hProcess) {
            std::wcout << L"[-] ���̾����Ч" << std::endl;
            return false;
        }

        // ���DLL�ļ��Ƿ����
        if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::wcout << L"[-] DLL�ļ�������: " << dllPath << std::endl;
            return false;
        }

        // ����·������
        SIZE_T pathSize = (dllPath.length() + 1) * sizeof(wchar_t);

        // ��Ŀ������з����ڴ�
        LPVOID remotePath = VirtualAllocEx(hProcess, NULL, pathSize,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath) {
            std::wcout << L"[-] �޷���Ŀ������з����ڴ棬������: " << GetLastError() << std::endl;
            return false;
        }

        std::wcout << L"[+] ��Ŀ������з����ڴ�: 0x" << std::hex << remotePath << std::endl;

        // д��DLL·����Ŀ�����
        if (!WriteProcessMemory(hProcess, remotePath, dllPath.c_str(), pathSize, NULL)) {
            std::wcout << L"[-] �޷�д��DLL·����Ŀ����̣�������: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] �ɹ�д��DLL·����Ŀ�����" << std::endl;

        // ��ȡLoadLibraryW������ַ
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!hKernel32) {
            std::wcout << L"[-] �޷���ȡkernel32.dll���" << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        LPTHREAD_START_ROUTINE loadLibraryAddr =
            (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
        if (!loadLibraryAddr) {
            std::wcout << L"[-] �޷���ȡLoadLibraryW��ַ" << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] LoadLibraryW��ַ: 0x" << std::hex << loadLibraryAddr << std::endl;

        // ����Զ���߳�ִ��LoadLibraryW
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryAddr,
            remotePath, 0, NULL);
        if (!hThread) {
            std::wcout << L"[-] �޷�����Զ���̣߳�������: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] �ɹ�����Զ���̣߳��ȴ�ִ�����..." << std::endl;

        // �ȴ��߳�ִ�����
        WaitForSingleObject(hThread, INFINITE);

        // ��ȡ�߳��˳��루LoadLibrary�ķ���ֵ����DLL�Ļ���ַ��
        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);

        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);

        if (exitCode == 0) {
            std::wcout << L"[-] DLLע��ʧ�ܣ�LoadLibrary����NULL" << std::endl;
            return false;
        }

        std::wcout << L"[+] DLLע��ɹ���DLL����ַ: 0x" << std::hex << exitCode << std::endl;
        return true;
    }

    // ����2��Shellcodeע�뷽ʽ
    bool InjectDLL_Shellcode(const std::wstring& dllPath) {
        if (!hProcess) {
            std::wcout << L"[-] ���̾����Ч" << std::endl;
            return false;
        }

        // ����shellcode������LoadLibraryW
        // ���ַ�ʽ���Ը��õؿ���ִ�����̺ʹ�����

        // ��ȡ��Ҫ��API��ַ
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        LPVOID loadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
        LPVOID getLastError = GetProcAddress(hKernel32, "GetLastError");

        if (!loadLibraryW || !getLastError) {
            std::wcout << L"[-] �޷���ȡ��Ҫ��API��ַ" << std::endl;
            return false;
        }

        // �����ڴ�洢DLL·��
        SIZE_T pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
        LPVOID remotePath = VirtualAllocEx(hProcess, NULL, pathSize,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath) {
            return false;
        }

        WriteProcessMemory(hProcess, remotePath, dllPath.c_str(), pathSize, NULL);

        // ����shellcode�ڴ�
        SIZE_T shellcodeSize = 1024;
        LPVOID remoteShellcode = VirtualAllocEx(hProcess, NULL, shellcodeSize,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!remoteShellcode) {
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            return false;
        }

        // ����shellcode��x64��
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

        // ����ʵ�ʵ�ַ
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

        // ����ʵ�ʵ�ַ
        *(DWORD*)(&shellcode[1]) = (DWORD)remotePath;
        *(DWORD*)(&shellcode[6]) = (DWORD)loadLibraryW;
#endif

        // д��shellcode
        if (!WriteProcessMemory(hProcess, remoteShellcode, shellcode.data(),
            shellcode.size(), NULL)) {
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            VirtualFreeEx(hProcess, remoteShellcode, 0, MEM_RELEASE);
            return false;
        }

        // ִ��shellcode
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE)remoteShellcode,
            NULL, 0, NULL);
        if (!hThread) {
            std::wcout << L"[-] �޷�����Զ���߳�ִ��shellcode��������: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
            VirtualFreeEx(hProcess, remoteShellcode, 0, MEM_RELEASE);
            return false;
        }

        std::wcout << L"[+] ͨ��shellcodeִ��DLLע��..." << std::endl;

        WaitForSingleObject(hThread, INFINITE);

        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);

        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, remoteShellcode, 0, MEM_RELEASE);

        if (exitCode == 0) {
            std::wcout << L"[-] Shellcodeע��ʧ��" << std::endl;
            return false;
        }

        std::wcout << L"[+] Shellcodeע��ɹ���DLL����ַ: 0x" << std::hex << exitCode << std::endl;
        return true;
    }

    // �����̼ܹ�ƥ��
    bool CheckArchitecture() {
        if (!hProcess) return false;

        BOOL isWow64Process = FALSE;
        BOOL isWow64Current = FALSE;

        IsWow64Process(hProcess, &isWow64Process);
        IsWow64Process(GetCurrentProcess(), &isWow64Current);

        if (isWow64Process != isWow64Current) {
            std::wcout << L"[-] ���棺���̼ܹ���ƥ�䣡" << std::endl;
            std::wcout << L"    Ŀ����� WOW64: " << (isWow64Process ? L"��" : L"��") << std::endl;
            std::wcout << L"    ��ǰ���� WOW64: " << (isWow64Current ? L"��" : L"��") << std::endl;
            return false;
        }

        return true;
    }

    // �г����н���
    void ListProcesses() {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        std::wcout << L"\n=== ��ǰ���еĽ��� ===" << std::endl;
        std::wcout << L"PID\t������" << std::endl;
        std::wcout << L"---\t------" << std::endl;

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wcout << pe32.th32ProcessID << L"\t" << pe32.szExeFile << std::endl;
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
    }
};

// ʹ��ʾ��
int wmain(int argc, wchar_t* argv[]) {
    setlocale(LC_ALL, "chs");

    std::wcout << L"=== DLLע���� - CreateRemoteThread�汾 ===" << std::endl;

    if (argc < 3) {
        std::wcout << L"�÷�: " << argv[0] << L" <��������PID> <DLL·��> [����]" << std::endl;
        std::wcout << L"����: 1=LoadLibrary(Ĭ��), 2=Shellcode" << std::endl;
        std::wcout << L"ʾ��: " << argv[0] << L" notepad.exe C:\\MyDLL.dll 1" << std::endl;
        return 1;
    }

    DLLInjector injector;

    // �����Ҫ�鿴�����б�
    if (wcscmp(argv[1], L"list") == 0) {
        injector.ListProcesses();
        return 0;
    }

    // ȷ��Ŀ�����PID
    DWORD targetPID = 0;
    if (iswdigit(argv[1][0])) {
        // ��������֣�����PID����
        targetPID = _wtoi(argv[1]);
    }
    else {
        // ������ַ�������������������
        targetPID = injector.FindProcessByName(argv[1]);
        if (targetPID == 0) {
            std::wcout << L"[-] δ�ҵ�����: " << argv[1] << std::endl;
            return 1;
        }
    }

    std::wcout << L"[+] Ŀ�����PID: " << targetPID << std::endl;

    // ��Ŀ�����
    if (!injector.OpenTargetProcess(targetPID)) {
        return 1;
    }

    // ���ܹ�ƥ��
    if (!injector.CheckArchitecture()) {
        std::wcout << L"[-] �ܹ���ƥ�䣬���ܵ���ע��ʧ��" << std::endl;
    }

    // ��ȡDLL·��
    std::wstring dllPath = argv[2];

    // ȷ��ע�뷽��
    int method = (argc >= 4) ? _wtoi(argv[3]) : 1;

    bool success = false;
    switch (method) {
    case 1:
        std::wcout << L"[*] ʹ��LoadLibrary����ע��..." << std::endl;
        success = injector.InjectDLL_LoadLibrary(dllPath);
        break;
    case 2:
        std::wcout << L"[*] ʹ��Shellcode����ע��..." << std::endl;
        success = injector.InjectDLL_Shellcode(dllPath);
        break;
    default:
        std::wcout << L"[-] ��֧�ֵ�ע�뷽��: " << method << std::endl;
        return 1;
    }

    if (success) {
        std::wcout << L"[+] DLLע����ɣ�" << std::endl;
        return 0;
    }
    else {
        std::wcout << L"[-] DLLע��ʧ�ܣ�" << std::endl;
        return 1;
    }
}