// 首先包含 winsock2.h
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>  // windows.h 必须在 winsock2.h 之后
#include <iphlpapi.h>
#include <lmcons.h>

// 然后包含 GDI+ 和其他头文件
#include <GdiPlus.h>
#include <atlimage.h>
#include <vector>
#include <string>
#include <time.h>

#include "watermark.h"



#pragma comment(lib,"Gdiplus.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define min(x, y) (x < y) ? x : y
#define max(x, y) (x > y) ? x : y

// 初始化静态成员
WatermarkManager* WatermarkManager::s_pInstance = nullptr;

WatermarkManager::WatermarkManager()
    : m_hwnd(NULL)
    , m_hInstance(NULL)
    , m_gdiplusToken(0)
    , m_initialized(false)
    , m_running(false)
{
    s_pInstance = this;
}

WatermarkManager::~WatermarkManager()
{
    StopWatermark();
    if (m_initialized) {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
    }
    s_pInstance = nullptr;
}

bool WatermarkManager::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    m_initialized = (status == Gdiplus::Ok);
    return m_initialized;
}

bool WatermarkManager::StartWatermark()
{
    if (!m_initialized || m_running) {
        return false;
    }

    // 创建窗口
    m_hwnd = CreateTransparentWindow(
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN)
    );

    if (!m_hwnd) {
        return false;
    }

    // 设置定时器，定期更新水印
    SetTimer(m_hwnd, TIMER_ID, 1000, NULL);

    // 立即更新一次水印
    UpdateWatermark();

    m_running = true;
    return true;
}

void WatermarkManager::StopWatermark()
{
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_ID);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    m_running = false;
}

// 在 UpdateWatermark 方法中，添加文本变量更新功能 - 只更新时间变量
void WatermarkManager::UpdateWatermark()
{
    if (m_hwnd && m_running) {
        // 检查文本中是否包含时间变量，如果有则更新
        if (m_params.text.find(L"%%datetime%%") != std::wstring::npos) {
            // 创建临时文本并处理变量
            std::wstring tempText = m_params.text;
            std::wstring::size_type pos = 0;

            // 仅替换时间变量，因为其他变量一般不会变化
            while ((pos = tempText.find(L"%%datetime%%", pos)) != std::wstring::npos) {
                tempText.replace(pos, 12, GetCurrentTimeString());
                pos += GetCurrentTimeString().length();
            }

            // 临时保存处理后的文本
            std::wstring originalText = m_params.text;
            m_params.text = tempText;

            // 绘制水印
            DrawMultiMonitorWatermark();

            // 恢复原始文本（包含变量）以便下次更新
            m_params.text = originalText;
        }
        else {
            // 没有时间变量，直接绘制
            DrawMultiMonitorWatermark();
        }
    }
}

LRESULT CALLBACK WatermarkManager::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (s_pInstance) {
        return s_pInstance->WndProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WatermarkManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID) {
            UpdateWatermark();
        }
        return 0;

    case WM_DESTROY:
        StopWatermark();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

HWND WatermarkManager::CreateTransparentWindow(int width, int height)
{
    // 注册窗口类
    const wchar_t* g_ClassName = L"WatermarkWindow";

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = g_ClassName;
    RegisterClassEx(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        g_ClassName,
        L"Watermark Window",
        WS_POPUP,
        0, 0, width, height,
        NULL, NULL, m_hInstance, NULL
    );

    if (!hwnd) {
        return NULL;
    }

    // 设置窗口样式
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE)
        & ~WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);

    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

// 枚举显示器回调函数
BOOL CALLBACK WatermarkManager::EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    std::vector<MonitorInfo>* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);

    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &monitorInfo);

    MonitorInfo info;
    info.hMonitor = hMonitor;
    info.rect = monitorInfo.rcMonitor;
    info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

    monitors->push_back(info);
    return TRUE;
}

// 获取所有显示器信息
std::vector<MonitorInfo> WatermarkManager::GetAllMonitors()
{
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

// 修改后的水印绘制函数，支持多显示器
void WatermarkManager::DrawMultiMonitorWatermark()
{
    // 获取所有显示器信息
    std::vector<MonitorInfo> monitors = GetAllMonitors();
    if (monitors.empty()) return;

    // 计算所有显示器的总体边界
    RECT totalBounds = monitors[0].rect;
    for (size_t i = 1; i < monitors.size(); i++) {
        totalBounds.left = min(totalBounds.left, monitors[i].rect.left);
        totalBounds.top = min(totalBounds.top, monitors[i].rect.top);
        totalBounds.right = max(totalBounds.right, monitors[i].rect.right);
        totalBounds.bottom = max(totalBounds.bottom, monitors[i].rect.bottom);
    }

    // 计算覆盖所有显示器的大小
    int totalWidth = totalBounds.right - totalBounds.left;
    int totalHeight = totalBounds.bottom - totalBounds.top;

    // 创建屏幕DC
    HDC screenDC = GetDC(NULL);

    // 创建兼容DC和32位ARGB位图
    HDC memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = totalWidth;
    bmi.bmiHeader.biHeight = -totalHeight; // 自上而下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;    // 32位ARGB
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = NULL;
    HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    // 初始化位图为全透明 (ARGB = 0x00000000)
    memset(bits, 0, totalWidth * totalHeight * 4);

    // 计算水印图像大小
    int watermarkSize = (int)(sqrt(totalWidth * totalWidth + totalHeight * totalHeight) * 1.2);

    // 创建水印DC和位图
    HDC watermarkDC = CreateCompatibleDC(screenDC);
    BITMAPINFO wmBmi = bmi;
    wmBmi.bmiHeader.biWidth = watermarkSize;
    wmBmi.bmiHeader.biHeight = -watermarkSize;

    void* wmBits = NULL;
    HBITMAP watermarkBitmap = CreateDIBSection(watermarkDC, &wmBmi, DIB_RGB_COLORS, &wmBits, NULL, 0);
    HBITMAP oldWatermarkBitmap = (HBITMAP)SelectObject(watermarkDC, watermarkBitmap);

    // 初始化水印位图为全透明
    memset(wmBits, 0, watermarkSize * watermarkSize * 4);

    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 从COLORREF提取RGB值
    int r = GetRValue(m_params.textColor);
    int g = GetGValue(m_params.textColor);
    int b = GetBValue(m_params.textColor);

    // 使用GDI+直接绘制带Alpha和指定颜色的文本
    Gdiplus::Graphics watermarkGraphics(watermarkDC);

    // 设置高质量文本渲染
    watermarkGraphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    watermarkGraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // 根据文本颜色和透明度选择合适的渲染方式
    if (m_params.textAlpha < 100) {
        watermarkGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    }
    else if (r > 250 && g > 250 && b > 250) {
        watermarkGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
    }
    else {
        watermarkGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    }

    Gdiplus::FontFamily fontFamily(m_params.fontName.c_str());
    Gdiplus::Font font(&fontFamily, (float)m_params.fontSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    // 为了处理低透明度情况，增加一个最小Alpha值
    int actualAlpha = m_params.textAlpha;
    if (actualAlpha < 30) actualAlpha = 30; // 确保至少有一点可见

    // 使用传入的颜色和透明度值
    Gdiplus::SolidBrush brush(Gdiplus::Color(actualAlpha, r, g, b));

    // 测量文本尺寸 (使用GDI+)
    Gdiplus::RectF layoutRect;
    Gdiplus::RectF boundingBox;
    watermarkGraphics.MeasureString(m_params.text.c_str(), m_params.text.length(), &font, Gdiplus::PointF(0, 0), &boundingBox);

    int textWidth = (int)boundingBox.Width;
    int textHeight = (int)boundingBox.Height;

    // 在水印DC上绘制网格状文本，使用单独的水平和垂直间距
    for (int y = -textHeight; y < watermarkSize + textHeight; y += textHeight + m_params.vSpacing) {
        for (int x = -textWidth; x < watermarkSize + textWidth; x += textWidth + m_params.hSpacing) {
            watermarkGraphics.DrawString(m_params.text.c_str(), m_params.text.length(), &font, Gdiplus::PointF((float)x, (float)y), &brush);
        }
    }

    // 只对高透明度的白色文本进行亮度过滤
    if (r > 250 && g > 250 && b > 250 && m_params.textAlpha > 100) {
        BYTE* pBits = (BYTE*)wmBits;
        for (int y = 0; y < watermarkSize; y++) {
            for (int x = 0; x < watermarkSize; x++) {
                int index = (y * watermarkSize + x) * 4;
                // RGB 分量
                BYTE pixelB = pBits[index];
                BYTE pixelG = pBits[index + 1];
                BYTE pixelR = pBits[index + 2];
                BYTE pixelA = pBits[index + 3];

                // 计算像素亮度 (简化加权)
                int brightness = (pixelR + pixelG + pixelB) / 3;

                // 只对暗边缘进行处理
                if (brightness < 180 && pixelA < 80) {
                    pBits[index + 3] = 0; // 设置为透明
                }
            }
        }
    }

    // 创建GDI+对象进行旋转操作
    Gdiplus::Bitmap* srcBitmap = new Gdiplus::Bitmap(watermarkSize, watermarkSize, watermarkSize * 4,
        PixelFormat32bppPARGB, (BYTE*)wmBits); // 使用预乘Alpha

    Gdiplus::Graphics memGraphics(memDC);

    // 设置高质量绘图模式
    memGraphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    memGraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // 使用和文本绘制相同的渲染设置
    if (m_params.textAlpha < 100) {
        memGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    }
    else if (r > 250 && g > 250 && b > 250) {
        memGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
    }
    else {
        memGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    }

    // 计算旋转中心（相对于整个虚拟屏幕的中心）
    int centerX = totalWidth / 2;
    int centerY = totalHeight / 2;

    // 创建变换矩阵并旋转 - 逆时针旋转
    Gdiplus::Matrix matrix;
    matrix.RotateAt(-m_params.angle, Gdiplus::PointF((float)centerX, (float)centerY));
    memGraphics.SetTransform(&matrix);

    // 计算绘制位置
    int drawX = centerX - watermarkSize / 2;
    int drawY = centerY - watermarkSize / 2;

    // 绘制旋转后的水印
    memGraphics.DrawImage(srcBitmap, drawX, drawY);

    // 获取窗口位置
    POINT ptDst = { totalBounds.left, totalBounds.top };

    // 设置分层窗口混合参数 
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;     // 使用每像素Alpha值
    blend.AlphaFormat = AC_SRC_ALPHA;    // 使用预乘Alpha

    SIZE sizeWnd = { totalWidth, totalHeight };
    POINT ptSrc = { 0, 0 };

    // 调整窗口大小以覆盖所有显示器
    SetWindowPos(m_hwnd, NULL, totalBounds.left, totalBounds.top,
        totalWidth, totalHeight, SWP_NOZORDER | SWP_NOACTIVATE);

    // 更新分层窗口
    UpdateLayeredWindow(
        m_hwnd,           // 目标窗口句柄
        screenDC,       // 屏幕DC
        &ptDst,         // 窗口位置
        &sizeWnd,       // 窗口大小
        memDC,          // 源DC
        &ptSrc,         // 源位置
        0,              // 颜色键（不使用）
        &blend,         // 混合函数
        ULW_ALPHA       // 使用Alpha通道
    );

    // 清理资源
    delete srcBitmap;
    Gdiplus::GdiplusShutdown(gdiplusToken);

    SelectObject(watermarkDC, oldWatermarkBitmap);
    DeleteObject(watermarkBitmap);
    DeleteDC(watermarkDC);

    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);
}

void WatermarkManager::SetWatermarkParams(const WatermarkParams& params)
{
    // 先复制参数
    m_params = params;

    // 处理文本中的变量（如果有的话）
    if (m_params.text.find(L"%%") != std::wstring::npos) {
        m_params.text = ProcessTextVariables(m_params.text);
    }
}

// 处理文本变量
std::wstring WatermarkManager::ProcessTextVariables(const std::wstring& text)
{
    std::wstring result = text;
    std::wstring::size_type pos = 0;

    // 替换时间变量
    while ((pos = result.find(L"%%datetime%%", pos)) != std::wstring::npos) {
        result.replace(pos, 12, GetCurrentTimeString());
        pos += GetCurrentTimeString().length();
    }

    // 替换主机名变量
    pos = 0;
    while ((pos = result.find(L"%%hostname%%", pos)) != std::wstring::npos) {
        std::wstring hostname = GetHostName();
        result.replace(pos, 12, hostname);
        pos += hostname.length();
    }

    // 替换IP地址变量
    pos = 0;
    while ((pos = result.find(L"%%ip%%", pos)) != std::wstring::npos) {
        std::wstring ip = GetIPAddress();
        result.replace(pos, 6, ip);
        pos += ip.length();
    }

    // 替换MAC地址变量
    pos = 0;
    while ((pos = result.find(L"%%mac%%", pos)) != std::wstring::npos) {
        std::wstring mac = GetMACAddress();
        result.replace(pos, 7, mac);
        pos += mac.length();
    }

    // 替换登录名变量
    pos = 0;
    while ((pos = result.find(L"%%loginname%%", pos)) != std::wstring::npos) {
        std::wstring loginname = GetLoginName();
        result.replace(pos, 13, loginname);
        pos += loginname.length();
    }

    return result;
}

// 获取当前时间字符串
std::wstring WatermarkManager::GetCurrentTimeString()
{
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);

    wchar_t timeStr[64];
    wcsftime(timeStr, sizeof(timeStr) / sizeof(wchar_t), L"%Y/%m/%d %H:%M", &localTime);

    return timeStr;
}

// 获取主机名
std::wstring WatermarkManager::GetHostName()
{
    wchar_t hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname) / sizeof(wchar_t);
    if (GetComputerNameW(hostname, &size)) {
        return hostname;
    }
    return L"Unknown";
}

// 获取IP地址
std::wstring WatermarkManager::GetIPAddress()
{
    // 使用 Windows Socket API 获取IP地址
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return L"0.0.0.0";
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        WSACleanup();
        return L"0.0.0.0";
    }

    struct addrinfo hints, * res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        WSACleanup();
        return L"0.0.0.0";
    }

    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);

    freeaddrinfo(res);
    WSACleanup();

    // 转换为宽字符
    size_t size = strlen(ip) + 1;
    wchar_t* w_ip = new wchar_t[size];
    size_t outSize;
    mbstowcs_s(&outSize, w_ip, size, ip, size - 1);
    std::wstring result(w_ip);
    delete[] w_ip;

    return result;
}

// 获取MAC地址
std::wstring WatermarkManager::GetMACAddress()
{
    // 首先获取IP地址
    std::wstring ipAddress = GetIPAddress();
    if (ipAddress == L"0.0.0.0") {
        return L"00:00:00:00:00:00";
    }

    // 使用IP地址相关的网络适配器来获取MAC
    IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    ULONG bufLen = sizeof(IP_ADAPTER_INFO);

    if (GetAdaptersInfo(pAdapterInfo, &bufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);
    }

    std::wstring macAddress = L"00:00:00:00:00:00";
    if (GetAdaptersInfo(pAdapterInfo, &bufLen) == NO_ERROR) {
        IP_ADAPTER_INFO* pAdapter = pAdapterInfo;
        while (pAdapter) {
            // 转换为宽字符
            size_t size = strlen(pAdapter->IpAddressList.IpAddress.String) + 1;
            wchar_t* w_ip = new wchar_t[size];
            size_t outSize;
            mbstowcs_s(&outSize, w_ip, size, pAdapter->IpAddressList.IpAddress.String, size - 1);
            std::wstring adapterIp(w_ip);
            delete[] w_ip;

            if (adapterIp == ipAddress) {
                // 找到匹配的适配器，提取MAC地址
                wchar_t mac[18];
                swprintf_s(mac, L"%02X:%02X:%02X:%02X:%02X:%02X",
                    pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2],
                    pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
                macAddress = mac;
                break;
            }

            pAdapter = pAdapter->Next;
        }
    }

    if (pAdapterInfo) {
        free(pAdapterInfo);
    }

    return macAddress;
}

// 获取当前登录用户名
std::wstring WatermarkManager::GetLoginName()
{
    wchar_t username[UNLEN + 1];
    DWORD size = sizeof(username) / sizeof(wchar_t);
    if (GetUserNameW(username, &size)) {
        return username;
    }
    return L"Unknown";
}
