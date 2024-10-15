#include <windows.h>
#include <iostream>
#include <fstream>

#include "resource.h"

#include "PEGenerator.h"

//BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
//    WCHAR tempFilePath[MAX_PATH];
//
//    // ������ʱ�ļ�·��
//    GetTempPath(MAX_PATH, tempFilePath);
//    GetTempFileName(tempFilePath, L"tmp", 0, tempFilePath);
//
//    // ����ǰ��ִ���ļ����Ƶ���ʱ�ļ�
//    if (!CopyFile(exePath, tempFilePath, FALSE)) {
//        std::wcerr << L"Failed to create temporary file.\n";
//        return FALSE;
//    }
//
//    // ����ʱ�ļ�������Դ����
//    HANDLE hUpdate = BeginUpdateResource(tempFilePath, FALSE);
//    if (!hUpdate) {
//        std::wcerr << L"Failed to open file for resource update.\n";
//        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
//        return FALSE;
//    }
//
//    // ��Ҫ��ӵ���Դ���ļ�
//    std::ifstream file(resourceFilePath, std::ios::binary);
//    if (!file.is_open()) {
//        std::wcerr << L"Failed to open the input file.\n";
//        EndUpdateResource(hUpdate, TRUE);
//        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
//        return FALSE;
//    }
//
//    // ��ȡ�ļ����ݵ��ڴ�
//    file.seekg(0, std::ios::end);
//    size_t fileSize = file.tellg();
//    file.seekg(0, std::ios::beg);
//
//    char* buffer = new char[fileSize];
//    file.read(buffer, fileSize);
//    file.close();
//
//    // ���ļ����ݸ��µ���Դ��
//    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_BINARY_FILE), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, fileSize)) {
//        std::wcerr << L"Failed to update the resource.\n";
//        delete[] buffer;
//        EndUpdateResource(hUpdate, TRUE);
//        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
//        return FALSE;
//    }
//
//    delete[] buffer;
//
//    // �����Դ����
//    if (!EndUpdateResource(hUpdate, FALSE)) {
//        std::wcerr << L"Failed to finalize resource update.\n";
//        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
//        return FALSE;
//    }
//
//    // ���޸ĺ����ʱ�ļ����Ƶ�Ŀ���ļ�·��
//    if (!CopyFile(tempFilePath, newExePath, FALSE)) {
//        std::wcerr << L"Failed to create the final output file.\n";
//        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
//        return FALSE;
//    }
//
//    // ɾ����ʱ�ļ�
//    DeleteFile(tempFilePath);
//
//    std::wcout << L"Resource added successfully.\n";
//    return TRUE;
//}
//
//
//BOOL ExtractAndRunResourceProgram(int resourceId) {
//    // ���ص�ǰģ��
//    HMODULE hModule = GetModuleHandle(NULL);
//    if (!hModule) {
//        std::wcerr << L"Failed to load module.\n";
//        return FALSE;
//    }
//
//    // ������Դ
//    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
//    if (!hResource) {
//        std::wcerr << L"Failed to find resource.\n";
//        return FALSE;
//    }
//
//    // ������Դ
//    HGLOBAL hResData = LoadResource(hModule, hResource);
//    if (!hResData) {
//        std::wcerr << L"Failed to load resource.\n";
//        return FALSE;
//    }
//
//    // ��ȡ��Դ��С������ָ��
//    DWORD dataSize = SizeofResource(hModule, hResource);
//    void* pData = LockResource(hResData);
//
//    // ����Դ����д����ʱ�ļ�
//    WCHAR tempPath[MAX_PATH];
//    WCHAR tempFile[MAX_PATH];
//    GetTempPath(MAX_PATH, tempPath);
//    GetTempFileName(tempPath, L"tmp", 0, tempFile);
//
//    std::ofstream outFile(tempFile, std::ios::binary);
//    if (!outFile.is_open()) {
//        std::wcerr << L"Failed to create temp file.\n";
//        return FALSE;
//    }
//    outFile.write((char*)pData, dataSize);
//    outFile.close();
//
//    // ����һ������������ȡ�ĳ���
//    STARTUPINFO si = { sizeof(si) };
//    PROCESS_INFORMATION pi;
//    if (!CreateProcess(tempFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
//        std::wcerr << L"Failed to run the extracted program.\n";
//        DeleteFile(tempFile); // ɾ����ʱ�ļ�
//        return FALSE;
//    }
//
//    // �ȴ��������
//    WaitForSingleObject(pi.hProcess, INFINITE);
//
//    // ����
//    CloseHandle(pi.hProcess);
//    CloseHandle(pi.hThread);
//    DeleteFile(tempFile); // ɾ����ʱ�ļ�
//
//    std::wcout << L"Program executed successfully.\n";
//    return TRUE;
//}

int wmain(int argc, wchar_t* argv[]) {
    
    CPEGenerator pe_gen;
    pe_gen.ParseParams(argc, argv);

    //if (argc < 3) {

    //    if (argc == 2 && wcscmp(argv[1], L"--run-resource") == 0) {
    //        // ʹ����Դ ID 101 ��Ϊʾ��
    //        if (ExtractAndRunResourceProgram(IDR_BINARY_FILE)) {
    //            std::wcout << L"Resource program executed successfully.\n";
    //        }
    //        else {
    //            std::wcout << L"Failed to execute resource program.\n";
    //        }
    //    }
    //    else {
    //        std::wcout << L"Usage: NewProgram.exe --run-resource\n";
    //    }

    //    std::wcout << L"Usage: AddResource <target_exe> <file_to_add> <output_exe>\n";
    //    return 1;
    //}
    //WCHAR exePath[MAX_PATH];
    //// ��ȡ��ǰ�����·��
    //DWORD result = GetModuleFileName(NULL, exePath, MAX_PATH);
    //LPCWSTR resourceFilePath = argv[1];
    //LPCWSTR newExePath = argv[2];

    //if (AddFileToResource(exePath, resourceFilePath, newExePath)) {
    //    std::wcout << L"Resource added to new executable file: " << newExePath << L"\n";
    //}
    //else {
    //    std::wcout << L"Failed to add resource.\n";
    //}

    return 0;
}
