#include <windows.h>
#include <iostream>
#include <fstream>

#include "resource.h"

#include "PEGenerator.h"

//BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
//    WCHAR tempFilePath[MAX_PATH];
//
//    // 创建临时文件路径
//    GetTempPath(MAX_PATH, tempFilePath);
//    GetTempFileName(tempFilePath, L"tmp", 0, tempFilePath);
//
//    // 将当前可执行文件复制到临时文件
//    if (!CopyFile(exePath, tempFilePath, FALSE)) {
//        std::wcerr << L"Failed to create temporary file.\n";
//        return FALSE;
//    }
//
//    // 打开临时文件进行资源更新
//    HANDLE hUpdate = BeginUpdateResource(tempFilePath, FALSE);
//    if (!hUpdate) {
//        std::wcerr << L"Failed to open file for resource update.\n";
//        DeleteFile(tempFilePath); // 删除临时文件
//        return FALSE;
//    }
//
//    // 打开要添加到资源的文件
//    std::ifstream file(resourceFilePath, std::ios::binary);
//    if (!file.is_open()) {
//        std::wcerr << L"Failed to open the input file.\n";
//        EndUpdateResource(hUpdate, TRUE);
//        DeleteFile(tempFilePath); // 删除临时文件
//        return FALSE;
//    }
//
//    // 读取文件内容到内存
//    file.seekg(0, std::ios::end);
//    size_t fileSize = file.tellg();
//    file.seekg(0, std::ios::beg);
//
//    char* buffer = new char[fileSize];
//    file.read(buffer, fileSize);
//    file.close();
//
//    // 将文件内容更新到资源中
//    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_BINARY_FILE), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, fileSize)) {
//        std::wcerr << L"Failed to update the resource.\n";
//        delete[] buffer;
//        EndUpdateResource(hUpdate, TRUE);
//        DeleteFile(tempFilePath); // 删除临时文件
//        return FALSE;
//    }
//
//    delete[] buffer;
//
//    // 完成资源更新
//    if (!EndUpdateResource(hUpdate, FALSE)) {
//        std::wcerr << L"Failed to finalize resource update.\n";
//        DeleteFile(tempFilePath); // 删除临时文件
//        return FALSE;
//    }
//
//    // 将修改后的临时文件复制到目标文件路径
//    if (!CopyFile(tempFilePath, newExePath, FALSE)) {
//        std::wcerr << L"Failed to create the final output file.\n";
//        DeleteFile(tempFilePath); // 删除临时文件
//        return FALSE;
//    }
//
//    // 删除临时文件
//    DeleteFile(tempFilePath);
//
//    std::wcout << L"Resource added successfully.\n";
//    return TRUE;
//}
//
//
//BOOL ExtractAndRunResourceProgram(int resourceId) {
//    // 加载当前模块
//    HMODULE hModule = GetModuleHandle(NULL);
//    if (!hModule) {
//        std::wcerr << L"Failed to load module.\n";
//        return FALSE;
//    }
//
//    // 查找资源
//    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
//    if (!hResource) {
//        std::wcerr << L"Failed to find resource.\n";
//        return FALSE;
//    }
//
//    // 加载资源
//    HGLOBAL hResData = LoadResource(hModule, hResource);
//    if (!hResData) {
//        std::wcerr << L"Failed to load resource.\n";
//        return FALSE;
//    }
//
//    // 获取资源大小和数据指针
//    DWORD dataSize = SizeofResource(hModule, hResource);
//    void* pData = LockResource(hResData);
//
//    // 将资源数据写入临时文件
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
//    // 创建一个进程运行提取的程序
//    STARTUPINFO si = { sizeof(si) };
//    PROCESS_INFORMATION pi;
//    if (!CreateProcess(tempFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
//        std::wcerr << L"Failed to run the extracted program.\n";
//        DeleteFile(tempFile); // 删除临时文件
//        return FALSE;
//    }
//
//    // 等待程序结束
//    WaitForSingleObject(pi.hProcess, INFINITE);
//
//    // 清理
//    CloseHandle(pi.hProcess);
//    CloseHandle(pi.hThread);
//    DeleteFile(tempFile); // 删除临时文件
//
//    std::wcout << L"Program executed successfully.\n";
//    return TRUE;
//}

int wmain(int argc, wchar_t* argv[]) {
    
    CPEGenerator pe_gen;
    pe_gen.ParseParams(argc, argv);

    //if (argc < 3) {

    //    if (argc == 2 && wcscmp(argv[1], L"--run-resource") == 0) {
    //        // 使用资源 ID 101 作为示例
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
    //// 获取当前程序的路径
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
