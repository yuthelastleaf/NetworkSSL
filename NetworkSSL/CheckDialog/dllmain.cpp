// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

// FileDialogMonitor_MinHook.cpp - 使用MinHook库监控文件对话框
#include <Windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <string>
#include <map>
#include <mutex>
#include <sstream>
#include <MinHook.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")

// 平台检测宏
#ifdef _WIN64
#define PLATFORM_NAME L"x64"
#else
#define PLATFORM_NAME L"x86"
#endif

// 全局变量
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
std::mutex g_logMutex;
std::map<HWND, std::wstring> g_dialogMap;
std::map<HWND, std::string> g_dialogMapA;
std::mutex g_dialogMutex;

void LogMessage(const std::wstring& message);

// 原始函数指针
typedef HWND(WINAPI* CreateWindowExW_t)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef BOOL(WINAPI* GetOpenFileNameW_t)(LPOPENFILENAMEW);
typedef BOOL(WINAPI* GetOpenFileNameA_t)(LPOPENFILENAMEA);  // 修正：使用 LPOPENFILENAMEA
typedef BOOL(WINAPI* GetSaveFileNameW_t)(LPOPENFILENAMEW);
typedef BOOL(WINAPI* GetSaveFileNameA_t)(LPOPENFILENAMEA);  // 添加 ANSI 版本
typedef INT_PTR(WINAPI* DialogBoxParamW_t)(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);
typedef INT_PTR(WINAPI* DialogBoxParamA_t)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);  // 添加 ANSI 版本
typedef HWND(WINAPI* CreateDialogParamW_t)(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);
typedef HRESULT(WINAPI* CoCreateInstance_t)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);

// 原始函数指针类型定义
typedef HWND(WINAPI* CreateWindowExA_t)(
    DWORD     dwExStyle,
    LPCSTR    lpClassName,
    LPCSTR    lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam
    );

typedef HWND(WINAPI* CreateDialogParamA_t)(
    HINSTANCE hInstance,
    LPCSTR    lpTemplateName,
    HWND      hWndParent,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam
    );

typedef INT_PTR(WINAPI* DialogBoxIndirectParamW_t)(
    HINSTANCE       hInstance,
    LPCDLGTEMPLATEW lpTemplate,
    HWND            hWndParent,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam
    );

typedef INT_PTR(WINAPI* DialogBoxIndirectParamA_t)(
    HINSTANCE       hInstance,
    LPCDLGTEMPLATEA lpTemplate,  // 添加 ANSI 版本
    HWND            hWndParent,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam
    );

// 原始函数指针存储
CreateWindowExW_t fpCreateWindowExW = nullptr;
CreateWindowExA_t fpCreateWindowExA = nullptr;
GetOpenFileNameW_t fpGetOpenFileNameW = nullptr;
GetOpenFileNameA_t fpGetOpenFileNameA = nullptr;
GetSaveFileNameW_t fpGetSaveFileNameW = nullptr;
GetSaveFileNameA_t fpGetSaveFileNameA = nullptr;
DialogBoxParamW_t fpDialogBoxParamW = nullptr;
DialogBoxParamA_t fpDialogBoxParamA = nullptr;
CreateDialogParamW_t fpCreateDialogParamW = nullptr;
CreateDialogParamA_t fpCreateDialogParamA = nullptr;
CoCreateInstance_t fpCoCreateInstance = nullptr;
DialogBoxIndirectParamW_t fpDialogBoxIndirectParamW = nullptr;
DialogBoxIndirectParamA_t fpDialogBoxIndirectParamA = nullptr;

// IFileDialog相关
const CLSID CLSID_FileOpenDialog = { 0xDC1C5A9C, 0xE88A, 0x4dde, {0xA5, 0xA1, 0x60, 0xF8, 0x2A, 0x20, 0xAE, 0xF7} };
const CLSID CLSID_FileSaveDialog = { 0xC0B4E2F3, 0xBA21, 0x4773, {0x8D, 0xBA, 0x33, 0x5E, 0xC9, 0x46, 0xEB, 0x8B} };

// ================================== 新增通过com接口创建对话框的方案 ===========================================
// IFileOpenDialog GetResult 相关声明

typedef HRESULT(WINAPI* GetResult_t)(IFileOpenDialog* This, IShellItem** ppsi);
GetResult_t fpGetResult = nullptr;

// IFileOpenDialog GetResults (多选) 相关声明  
typedef HRESULT(WINAPI* GetResults_t)(IFileOpenDialog* This, IShellItemArray** ppenum);
GetResults_t fpGetResults = nullptr;

// Hook GetResult (单选)
HRESULT WINAPI HookedGetResult(IFileOpenDialog* This, IShellItem** ppsi)
{
    LogMessage(L"[HookedGetResult] HookedGetResult enter");
    HRESULT hr = fpGetResult(This, ppsi);

    if (SUCCEEDED(hr) && ppsi && *ppsi) {
        PWSTR pszFilePath = nullptr;
        HRESULT hrPath = (*ppsi)->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

        if (SUCCEEDED(hrPath) && pszFilePath) {
            LogMessage(L"[GetResult] 选择的文件");
            LogMessage(pszFilePath);
            CoTaskMemFree(pszFilePath);
        }
    }

    return hr;
}

// Hook GetResults (多选)
HRESULT WINAPI HookedGetResults(IFileOpenDialog* This, IShellItemArray** ppenum)
{
    LogMessage(L"[HookedGetResults] HookedGetResults enter");
    HRESULT hr = fpGetResults(This, ppenum);

    if (SUCCEEDED(hr) && ppenum && *ppenum) {
        DWORD itemCount = 0;
        (*ppenum)->GetCount(&itemCount);

        LogMessage(L"[GetResults] 选择了多个文件");

        for (DWORD i = 0; i < itemCount; i++) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED((*ppenum)->GetItemAt(i, &pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    LogMessage(L"[GetResults] 文件");
                    LogMessage(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
    }

    return hr;
}

// DialogBoxParamW 相关声明
//typedef INT_PTR(WINAPI* DialogBoxParamW_t)(HINSTANCE hInstance, LPCWSTR lpTemplateName,
//    HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
//DialogBoxParamW_t fpDialogBoxParamW = nullptr;

// Hook DialogBoxParamW
INT_PTR WINAPI HookedDialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName,
    HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    LogMessage(L"[HookedDialogBoxParamW] HookedDialogBoxParamW enter");
    // 检查是否可能是文件对话框模板
    BOOL isFileDialog = FALSE;
    if (IS_INTRESOURCE(lpTemplateName)) {
        DWORD templateId = (DWORD)(ULONG_PTR)lpTemplateName;
        // 常见的文件对话框模板ID (这些值可能因Windows版本而异)
        if (templateId == 1024 || templateId == 1025 || templateId == 1026) {
            isFileDialog = TRUE;
            LogMessage(L"[DialogBoxParamW] 检测到可能的文件对话框, 模板ID");
        }
    }

    INT_PTR result = fpDialogBoxParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

    // 如果是文件对话框且用户点击了确定 (返回值通常是IDOK)
    if (isFileDialog && result == IDOK) {
        LogMessage(L"[DialogBoxParamW] 文件对话框返回确定, 结果");

        // 可以尝试通过窗口句柄获取更多信息
        if (hWndParent) {
            WCHAR windowText[MAX_PATH];
            if (GetWindowTextW(hWndParent, windowText, MAX_PATH)) {
                LogMessage(L"[DialogBoxParamW] 父窗口标题:");
                LogMessage(windowText);
            }
        }
    }

    return result;
}

// ==============================================================================================================

// 日志函数
void LogMessage(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    if (g_hLogFile == INVALID_HANDLE_VALUE) return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t buffer[2048];
    int len = swprintf_s(buffer, 2048, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] [TID:%04X] [%s] %s\r\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        GetCurrentThreadId(),
        PLATFORM_NAME,
        message.c_str());

    DWORD bytesWritten = 0;
    WriteFile(g_hLogFile, buffer, len * sizeof(wchar_t), &bytesWritten, NULL);
    FlushFileBuffers(g_hLogFile);
}

void LogMessageA(const std::string& message) {
    // 转换为宽字符并调用 LogMessage
    int len = MultiByteToWideChar(CP_ACP, 0, message.c_str(), -1, NULL, 0);
    if (len > 0) {
        std::wstring wmessage(len - 1, 0);
        MultiByteToWideChar(CP_ACP, 0, message.c_str(), -1, &wmessage[0], len);
        LogMessage(wmessage);
    }
}

// 获取窗口类名
std::wstring GetWindowClassName(HWND hwnd) {
    wchar_t className[256] = { 0 };
    GetClassNameW(hwnd, className, 256);
    return std::wstring(className);
}

std::string GetWindowClassNameA(HWND hwnd) {
    char className[256] = { 0 };
    GetClassNameA(hwnd, className, 256);
    return std::string(className);
}

// 获取窗口标题
std::wstring GetWindowTitle(HWND hwnd) {
    wchar_t title[512] = { 0 };
    GetWindowTextW(hwnd, title, 512);
    return std::wstring(title);
}

std::string GetWindowTitleA(HWND hwnd) {
    char title[512] = { 0 };
    GetWindowTextA(hwnd, title, 512);
    return std::string(title);
}

// 检查是否是文件对话框
bool IsFileDialog(HWND hwnd, const std::wstring& className) {
    if (className == L"#32770") {
        // 检查是否有文件对话框特征控件
        HWND hListView = FindWindowExW(hwnd, NULL, L"SHELLDLL_DefView", NULL);
        HWND hComboBox = FindWindowExW(hwnd, NULL, L"ComboBoxEx32", NULL);

        if (hListView || hComboBox) {
            return true;
        }

        // 检查标题
        std::wstring title = GetWindowTitle(hwnd);
        if (title.find(L"打开") != std::wstring::npos ||
            title.find(L"保存") != std::wstring::npos ||
            title.find(L"另存为") != std::wstring::npos ||
            title.find(L"Open") != std::wstring::npos ||
            title.find(L"Save") != std::wstring::npos ||
            title.find(L"Browse") != std::wstring::npos) {
            return true;
        }
    }

    return false;
}

bool IsFileDialogA(HWND hwnd, const std::string& className) {
    if (className == "#32770") {
        // 检查是否有文件对话框特征控件
        HWND hListView = FindWindowExA(hwnd, NULL, "SHELLDLL_DefView", NULL);
        HWND hComboBox = FindWindowExA(hwnd, NULL, "ComboBoxEx32", NULL);

        if (hListView || hComboBox) {
            return true;
        }

        // 检查标题
        std::string title = GetWindowTitleA(hwnd);
        if (title.find("打开") != std::string::npos ||
            title.find("保存") != std::string::npos ||
            title.find("另存为") != std::string::npos ||
            title.find("Open") != std::string::npos ||
            title.find("Save") != std::string::npos ||
            title.find("Browse") != std::string::npos) {
            return true;
        }
    }

    return false;
}

// Hook: DialogBoxIndirectParamW
INT_PTR WINAPI HookedDialogBoxIndirectParamW(
    HINSTANCE       hInstance,
    LPCDLGTEMPLATEW lpTemplate,
    HWND            hWndParent,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam
) {
    LogMessage(L"HookedDialogBoxIndirectParamW enter");

    // 检查是否是文件对话框（通过窗口样式/模板分析）
    if (lpTemplate != NULL) {
        // 标准文件对话框通常使用特定样式（DS_SHELLFONT | WS_CHILD | WS_VISIBLE...）
        if (lpTemplate->style & DS_SHELLFONT) {
            LogMessage(L"[Hook] Detected file dialog (DialogBoxIndirectParamW)!");
        }
    }

    if (fpDialogBoxIndirectParamW) {
        // 调用原始函数
        INT_PTR dia_res =  fpDialogBoxIndirectParamW(
            hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam
        );
        LogMessage(L"HookedDialogBoxIndirectParamW exit");
        MessageBox(NULL, L"等待，我在默默的等待", L"Hook", MB_OK);
        return dia_res;
    }
    else {
        return 0;  // 返回 0 而不是 NULL
    }
}

// Hook: CreateWindowExW
HWND WINAPI HookedCreateWindowExW(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam)
{
    // 调用原始函数
    HWND hwnd = fpCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle,
        X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    // 检查是否是对话框
    if (hwnd && lpClassName) {
        std::wstring className;
        if (HIWORD(lpClassName)) {
            className = lpClassName;
        }
        else {
            className = GetWindowClassName(hwnd);
        }

        if (IsFileDialog(hwnd, className)) {
            std::wstring title = lpWindowName ? lpWindowName : GetWindowTitle(hwnd);

            std::wstringstream ss;
            ss << L"[CreateWindowExW] 检测到文件对话框 - HWND: 0x" << std::hex << hwnd
                << L", 类名: " << className
                << L", 标题: " << title;
            LogMessage(ss.str());

            std::lock_guard<std::mutex> lock(g_dialogMutex);
            g_dialogMap[hwnd] = title;
        }
    }

    return hwnd;
}

// Hook: CreateWindowExA
HWND WINAPI HookedCreateWindowExA(
    DWORD     dwExStyle,
    LPCSTR    lpClassName,
    LPCSTR    lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam
)
{
    // 调用原始函数
    HWND hwnd = fpCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle,
        X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    // 检查是否是对话框
    if (hwnd && lpClassName) {
        std::string className;
        if (HIWORD(lpClassName)) {
            className = lpClassName;
        }
        else {
            className = GetWindowClassNameA(hwnd);
        }

        if (IsFileDialogA(hwnd, className)) {
            std::string title = lpWindowName ? lpWindowName : GetWindowTitleA(hwnd);

            std::stringstream ss;
            ss << "[CreateWindowExA] 检测到文件对话框 - HWND: 0x" << std::hex << hwnd
                << ", 类名: " << className
                << ", 标题: " << title;
            LogMessageA(ss.str());

            std::lock_guard<std::mutex> lock(g_dialogMutex);
            g_dialogMapA[hwnd] = title;
        }
    }

    return hwnd;
}

// Hook: GetOpenFileNameW
BOOL WINAPI HookedGetOpenFileNameW(LPOPENFILENAMEW lpofn) {
    LogMessage(L"[GetOpenFileNameW] 调用打开文件对话框");

    if (lpofn) {
        std::wstringstream ss;
        ss << L"[GetOpenFileNameW] 参数 - "
            << L"过滤器: " << (lpofn->lpstrFilter ? lpofn->lpstrFilter : L"(null)")
            << L", 初始目录: " << (lpofn->lpstrInitialDir ? lpofn->lpstrInitialDir : L"(null)")
            << L", 标题: " << (lpofn->lpstrTitle ? lpofn->lpstrTitle : L"(默认)");
        LogMessage(ss.str());
    }

    // 调用原始函数
    BOOL result = fpGetOpenFileNameW(lpofn);

    // 记录结果
    if (result && lpofn && lpofn->lpstrFile) {
        std::wstringstream ss;
        ss << L"[GetOpenFileNameW] 用户选择了文件: " << lpofn->lpstrFile;

        // 如果选择了多个文件
        if (lpofn->Flags & OFN_ALLOWMULTISELECT) {
            WCHAR* p = lpofn->lpstrFile;
            std::wstring dir = p;
            p += wcslen(p) + 1;

            if (*p) {
                ss << L" (多选模式，目录: " << dir << L")";
                while (*p) {
                    ss << L"\n  - " << p;
                    p += wcslen(p) + 1;
                }
            }
        }

        LogMessage(ss.str());
    }
    else {
        LogMessage(L"[GetOpenFileNameW] 用户取消了选择");
    }

    return result;
}

// Hook: GetOpenFileNameA (修正版本)
BOOL WINAPI HookedGetOpenFileNameA(LPOPENFILENAMEA lpofn) {
    LogMessage(L"[GetOpenFileNameA] 调用打开文件对话框");

    if (lpofn) {
        std::stringstream ss;
        ss << "[GetOpenFileNameA] 参数 - "
            << "过滤器: " << (lpofn->lpstrFilter ? lpofn->lpstrFilter : "(null)")
            << ", 初始目录: " << (lpofn->lpstrInitialDir ? lpofn->lpstrInitialDir : "(null)")
            << ", 标题: " << (lpofn->lpstrTitle ? lpofn->lpstrTitle : "(默认)");
        LogMessageA(ss.str());
    }

    // 调用原始函数
    BOOL result = fpGetOpenFileNameA(lpofn);

    // 记录结果
    if (result && lpofn && lpofn->lpstrFile) {
        std::stringstream ss;
        ss << "[GetOpenFileNameA] 用户选择了文件: " << lpofn->lpstrFile;

        // 如果选择了多个文件
        if (lpofn->Flags & OFN_ALLOWMULTISELECT) {
            char* p = lpofn->lpstrFile;
            std::string dir = p;
            p += strlen(p) + 1;

            if (*p) {
                ss << " (多选模式，目录: " << dir << ")";
                while (*p) {
                    ss << "\n  - " << p;
                    p += strlen(p) + 1;
                }
            }
        }

        LogMessageA(ss.str());
    }
    else {
        LogMessage(L"[GetOpenFileNameA] 用户取消了选择");
    }

    return result;
}

// Hook: GetSaveFileNameW
BOOL WINAPI HookedGetSaveFileNameW(LPOPENFILENAMEW lpofn) {
    LogMessage(L"[GetSaveFileNameW] 调用保存文件对话框");

    if (lpofn && lpofn->lpstrTitle) {
        std::wstringstream ss;
        ss << L"[GetSaveFileNameW] 标题: " << lpofn->lpstrTitle;
        LogMessage(ss.str());
    }

    // 调用原始函数
    BOOL result = fpGetSaveFileNameW(lpofn);

    // 记录结果
    if (result && lpofn && lpofn->lpstrFile) {
        std::wstringstream ss;
        ss << L"[GetSaveFileNameW] 用户选择保存到: " << lpofn->lpstrFile;
        LogMessage(ss.str());
    }
    else {
        LogMessage(L"[GetSaveFileNameW] 用户取消了保存");
    }

    return result;
}

// Hook: GetSaveFileNameA
BOOL WINAPI HookedGetSaveFileNameA(LPOPENFILENAMEA lpofn) {
    LogMessage(L"[GetSaveFileNameA] 调用保存文件对话框");

    if (lpofn && lpofn->lpstrTitle) {
        std::stringstream ss;
        ss << "[GetSaveFileNameA] 标题: " << lpofn->lpstrTitle;
        LogMessageA(ss.str());
    }

    // 调用原始函数
    BOOL result = fpGetSaveFileNameA(lpofn);

    // 记录结果
    if (result && lpofn && lpofn->lpstrFile) {
        std::stringstream ss;
        ss << "[GetSaveFileNameA] 用户选择保存到: " << lpofn->lpstrFile;
        LogMessageA(ss.str());
    }
    else {
        LogMessage(L"[GetSaveFileNameA] 用户取消了保存");
    }

    return result;
}

// Hook: DialogBoxParamW
//INT_PTR WINAPI HookedDialogBoxParamW(
//    HINSTANCE hInstance,
//    LPCWSTR lpTemplate,
//    HWND hWndParent,
//    DLGPROC lpDialogFunc,
//    LPARAM dwInitParam)
//{
//    std::wstringstream ss;
//    ss << L"[DialogBoxParamW] 模态对话框 - "
//        << L"模板: " << (HIWORD(lpTemplate) ? lpTemplate : L"(资源ID)")
//        << L", 父窗口: 0x" << std::hex << hWndParent;
//    LogMessage(ss.str());
//
//    return fpDialogBoxParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
//}

// Hook: CreateDialogParamW
HWND WINAPI HookedCreateDialogParamW(
    HINSTANCE hInstance,
    LPCWSTR lpTemplate,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam)
{
    HWND hwnd = fpCreateDialogParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);

    if (hwnd) {
        std::wstring className = GetWindowClassName(hwnd);
        if (className == L"#32770") {
            std::wstringstream ss;
            ss << L"[CreateDialogParamW] 非模态对话框 - HWND: 0x" << std::hex << hwnd;
            LogMessage(ss.str());
        }
    }

    return hwnd;
}

// Hook: CreateDialogParamA
HWND WINAPI HookedCreateDialogParamA(
    HINSTANCE hInstance,
    LPCSTR lpTemplate,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam)
{
    HWND hwnd = fpCreateDialogParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);

    if (hwnd) {
        std::string className = GetWindowClassNameA(hwnd);
        if (className == "#32770") {
            std::stringstream ss;
            ss << "[CreateDialogParamA] 非模态对话框 - HWND: 0x" << std::hex << hwnd;
            LogMessageA(ss.str());
        }
    }

    return hwnd;
}

// Hook: CoCreateInstance (用于捕获IFileDialog)
//HRESULT WINAPI HookedCoCreateInstance(
//    REFCLSID rclsid,
//    LPUNKNOWN pUnkOuter,
//    DWORD dwClsContext,
//    REFIID riid,
//    LPVOID* ppv)
//{
//    // 调用原始函数
//    HRESULT hr = fpCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
//
//    // 检查是否是文件对话框
//    if (SUCCEEDED(hr) && ppv && *ppv) {
//        if (IsEqualCLSID(rclsid, CLSID_FileOpenDialog)) {
//            LogMessage(L"[CoCreateInstance] 创建了 IFileOpenDialog");
//        }
//        else if (IsEqualCLSID(rclsid, CLSID_FileSaveDialog)) {
//            LogMessage(L"[CoCreateInstance] 创建了 IFileSaveDialog");
//        }
//    }
//
//    return hr;
//}

HRESULT WINAPI HookedCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    LPVOID* ppv)
{
    HRESULT hr = fpCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

    if (SUCCEEDED(hr) && ppv && *ppv) {
        if (IsEqualCLSID(rclsid, CLSID_FileOpenDialog)) {
            LogMessage(L"[CoCreateInstance] 创建了 IFileOpenDialog");

            // Hook VTable 方法
            IFileOpenDialog* pDialog = (IFileOpenDialog*)*ppv;
            void** vtable = *(void***)pDialog;

            // GetResult 通常在 vtable 索引 47 位置 (可能需要根据实际情况调整)
            if (!fpGetResult) {
                fpGetResult = (GetResult_t)vtable[47];

                DWORD oldProtect;
                VirtualProtect(&vtable[47], sizeof(void*), PAGE_READWRITE, &oldProtect);
                vtable[47] = (void*)HookedGetResult;
                VirtualProtect(&vtable[47], sizeof(void*), oldProtect, &oldProtect);
            }

            // GetResults 通常在 vtable 索引 48 位置 (可能需要根据实际情况调整)
            if (!fpGetResults) {
                fpGetResults = (GetResults_t)vtable[48];

                DWORD oldProtect;
                VirtualProtect(&vtable[48], sizeof(void*), PAGE_READWRITE, &oldProtect);
                vtable[48] = (void*)HookedGetResults;
                VirtualProtect(&vtable[48], sizeof(void*), oldProtect, &oldProtect);
            }
        }
        else if (IsEqualCLSID(rclsid, CLSID_FileSaveDialog)) {
            LogMessage(L"[CoCreateInstance] 创建了 IFileSaveDialog");
            // IFileSaveDialog 也有类似的方法可以 Hook
        }
    }

    return hr;
}

// 安装所有Hooks
bool InstallHooks() {
    // 初始化MinHook
    if (MH_Initialize() != MH_OK) {
        LogMessage(L"[ERROR] MinHook初始化失败");
        return false;
    }

    LogMessage(L"[系统] MinHook初始化成功");

    // Hook CreateWindowExW
    if (MH_CreateHookApi(L"user32", "CreateWindowExW", HookedCreateWindowExW,
        reinterpret_cast<LPVOID*>(&fpCreateWindowExW)) == MH_OK) {
        LogMessage(L"[成功] CreateWindowExW Hook创建成功");
    }

    // Hook CreateWindowExA
    if (MH_CreateHookApi(L"user32", "CreateWindowExA", HookedCreateWindowExA,
        reinterpret_cast<LPVOID*>(&fpCreateWindowExA)) == MH_OK) {
        LogMessage(L"[成功] CreateWindowExA Hook创建成功");
    }

    // Hook GetOpenFileNameW
    if (MH_CreateHookApi(L"comdlg32", "GetOpenFileNameW", HookedGetOpenFileNameW,
        reinterpret_cast<LPVOID*>(&fpGetOpenFileNameW)) == MH_OK) {
        LogMessage(L"[成功] GetOpenFileNameW Hook创建成功");
    }

    // Hook GetOpenFileNameA
    if (MH_CreateHookApi(L"comdlg32", "GetOpenFileNameA", HookedGetOpenFileNameA,
        reinterpret_cast<LPVOID*>(&fpGetOpenFileNameA)) == MH_OK) {
        LogMessage(L"[成功] GetOpenFileNameA Hook创建成功");
    }

    // Hook GetSaveFileNameW
    if (MH_CreateHookApi(L"comdlg32", "GetSaveFileNameW", HookedGetSaveFileNameW,
        reinterpret_cast<LPVOID*>(&fpGetSaveFileNameW)) == MH_OK) {
        LogMessage(L"[成功] GetSaveFileNameW Hook创建成功");
    }

    // Hook GetSaveFileNameA
    if (MH_CreateHookApi(L"comdlg32", "GetSaveFileNameA", HookedGetSaveFileNameA,
        reinterpret_cast<LPVOID*>(&fpGetSaveFileNameA)) == MH_OK) {
        LogMessage(L"[成功] GetSaveFileNameA Hook创建成功");
    }

    // Hook DialogBoxParamW
    if (MH_CreateHookApi(L"user32", "DialogBoxParamW", HookedDialogBoxParamW,
        reinterpret_cast<LPVOID*>(&fpDialogBoxParamW)) == MH_OK) {
        LogMessage(L"[成功] DialogBoxParamW Hook创建成功");
    }

    // Hook CreateDialogParamW
    if (MH_CreateHookApi(L"user32", "CreateDialogParamW", HookedCreateDialogParamW,
        reinterpret_cast<LPVOID*>(&fpCreateDialogParamW)) == MH_OK) {
        LogMessage(L"[成功] CreateDialogParamW Hook创建成功");
    }

    // Hook CreateDialogParamA
    if (MH_CreateHookApi(L"user32", "CreateDialogParamA", HookedCreateDialogParamA,
        reinterpret_cast<LPVOID*>(&fpCreateDialogParamA)) == MH_OK) {
        LogMessage(L"[成功] CreateDialogParamA Hook创建成功");
    }

    // Hook CoCreateInstance
    if (MH_CreateHookApi(L"ole32", "CoCreateInstance", HookedCoCreateInstance,
        reinterpret_cast<LPVOID*>(&fpCoCreateInstance)) == MH_OK) {
        LogMessage(L"[成功] CoCreateInstance Hook创建成功");
    }

    // Hook DialogBoxIndirectParamW
    if (MH_CreateHookApi(L"user32", "DialogBoxIndirectParamW", HookedDialogBoxIndirectParamW,
        reinterpret_cast<LPVOID*>(&fpDialogBoxIndirectParamW)) == MH_OK) {
        LogMessage(L"[成功] DialogBoxIndirectParamW Hook创建成功");
    }

    // 启用所有Hooks
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        LogMessage(L"[ERROR] 启用Hooks失败");
        return false;
    }

    LogMessage(L"[系统] 所有Hooks已启用");
    return true;
}

// 卸载所有Hooks
void UninstallHooks() {
    LogMessage(L"[系统] 开始卸载Hooks");

    // 禁用所有Hooks
    MH_DisableHook(MH_ALL_HOOKS);

    // 反初始化MinHook
    MH_Uninitialize();

    LogMessage(L"[系统] Hooks已卸载");
}

// 初始化
void Initialize() {
    // 创建日志文件
    wchar_t logPath[MAX_PATH];
    GetTempPathW(MAX_PATH, logPath);

    // 为不同平台创建不同的日志文件名
    wchar_t logFileName[MAX_PATH];
    swprintf_s(logFileName, MAX_PATH, L"FileDialogMonitor_MinHook_%s.log", PLATFORM_NAME);
    wcscat_s(logPath, logFileName);

    g_hLogFile = CreateFileW(
        logPath,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        // 写入 UTF-16 LE BOM
        WORD bom = 0xFEFF;
        DWORD written;
        WriteFile(g_hLogFile, &bom, sizeof(bom), &written, NULL);
    }

    LogMessage(L"=== 文件对话框监控DLL (MinHook版本) ===");

    // 记录平台信息
    std::wstringstream ss;
    ss << L"平台: " << PLATFORM_NAME << L" (" << (sizeof(void*) * 8) << L"位)";
    LogMessage(ss.str());

    wchar_t processName[MAX_PATH];
    GetModuleFileNameW(NULL, processName, MAX_PATH);
    ss.str(L"");
    ss << L"进程: " << processName << L" (PID: " << GetCurrentProcessId() << L")";
    LogMessage(ss.str());

    ss.str(L"");
    ss << L"日志文件: " << logPath;
    LogMessage(ss.str());

    // 安装Hooks
    InstallHooks();
}

// 清理
void Cleanup() {
    UninstallHooks();

    LogMessage(L"=== 文件对话框监控DLL已卸载 ===");

    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
}

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        Initialize();
        break;

    case DLL_PROCESS_DETACH:
        Cleanup();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

// 导出函数
extern "C" {
    __declspec(dllexport) void RefreshHooks() {
        LogMessage(L"[导出函数] RefreshHooks被调用");
        MH_DisableHook(MH_ALL_HOOKS);
        MH_EnableHook(MH_ALL_HOOKS);
    }

    __declspec(dllexport) int GetHookStatus() {
        return MH_OK;
    }

    __declspec(dllexport) const wchar_t* GetPlatformInfo() {
        static wchar_t info[64];
        swprintf_s(info, 64, L"%s (%d-bit)", PLATFORM_NAME, sizeof(void*) * 8);
        return info;
    }
}