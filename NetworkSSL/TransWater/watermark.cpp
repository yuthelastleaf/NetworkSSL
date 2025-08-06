// 改进的水印系统 - 支持新的点阵协议


#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <lmcons.h>
#include <GdiPlus.h>
#include <sstream>
#include <iostream>
#include <bitset>
#include "watermark.h"

#pragma comment(lib,"Gdiplus.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define min(x, y) (x < y) ? x : y
#define max(x, y) (x > y) ? x : y

// 初始化静态成员
ImprovedWatermarkManager* ImprovedWatermarkManager::s_pInstance = nullptr;

ImprovedWatermarkManager::ImprovedWatermarkManager()
    : m_hwnd(NULL)
    , m_hInstance(NULL)
    , m_gdiplusToken(0)
    , m_initialized(false)
    , m_running(false)
    , m_isDotMode(false)
    , m_isBlinking(false)
    , m_blinkCount(0)
{
    s_pInstance = this;

    // 初始化改进的点阵水印默认参数
    m_dotParams.ipAddress = "192.168.1.100";
    m_dotParams.dotRadius = 3;
    m_dotParams.dotSpacing = 15;
    m_dotParams.blockSpacing = 100;
    m_dotParams.headerColor = RGB(255, 0, 0);      // 红色头部行
    m_dotParams.dataColor = RGB(0, 100, 255);      // 蓝色数据行
    m_dotParams.opacity = 0.6f;
    m_dotParams.rotation = 15.0f;
    m_dotParams.hSpacing = 120;
    m_dotParams.vSpacing = 100;
    m_dotParams.showDebugInfo = false;
    m_dotParams.enableBlinking = true;
    m_dotParams.blinkDuration = 2000;
}

ImprovedWatermarkManager::~ImprovedWatermarkManager()
{
    StopWatermark();
    if (m_initialized) {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
    }
    s_pInstance = nullptr;
}

bool ImprovedWatermarkManager::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    m_initialized = (status == Gdiplus::Ok);
    return m_initialized;
}

bool ImprovedWatermarkManager::StartImprovedDotWatermark()
{
    if (!m_initialized || m_running) {
        return false;
    }

    m_isDotMode = true;

    // 编码IP地址为改进协议矩阵
    m_currentMatrix = EncodeIPToImprovedProtocol(m_dotParams.ipAddress);
    if (!m_currentMatrix.isValid) {
        std::cout << "IP地址编码失败: " << m_dotParams.ipAddress << std::endl;
        return false;
    }

    // 记录当前IP用于变化检测
    m_lastIP = m_dotParams.ipAddress;

    // 创建窗口
    m_hwnd = CreateTransparentWindow(
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN)
    );

    if (!m_hwnd) {
        return false;
    }

    // 设置定时器，用于定期更新水印和检测IP变化
    SetTimer(m_hwnd, TIMER_ID, 3000, NULL);  // 每3秒检查一次IP变化

    // 立即更新一次点阵水印
    UpdateDotWatermark();

    m_running = true;

    std::cout << "改进点阵水印启动成功" << std::endl;
    std::cout << "IP: " << m_currentMatrix.originalIP << std::endl;
    std::cout << "校验和: " << (int)m_currentMatrix.checksum << std::endl;

    return true;
}

// IP编码为改进协议
ImprovedWatermarkManager::EncodedMatrix ImprovedWatermarkManager::EncodeIPToImprovedProtocol(const std::string& ip)
{
    EncodedMatrix result;
    result.originalIP = ip;

    // 解析IP地址
    std::vector<uint8_t> ipBytes = ParseIP(ip);
    if (ipBytes.size() != 4) {
        std::cout << "无效的IP地址格式: " << ip << std::endl;
        return result;
    }

    // 计算校验和（4位）
    uint32_t sum = 0;
    for (uint8_t byte : ipBytes) {
        sum += byte;
    }
    uint8_t checksum = sum & 0x0F; // 只取低4位
    result.checksum = checksum;

    std::cout << "\n=== 编码IP地址 ===" << std::endl;
    std::cout << "IP: " << ip << std::endl;
    std::cout << "字节: ";
    for (int i = 0; i < 4; i++) {
        std::cout << (int)ipBytes[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "校验和: " << (int)checksum << " (0x" << std::hex << (int)checksum << std::dec << ")" << std::endl;

    // 第0行：构建头部 [同步位1010][校验和4bit]
    uint8_t headerByte = SYNC_PATTERN | checksum;
    FillMatrixRow(result.matrix[HEADER_ROW], headerByte);

    std::cout << "头部字节: 0x" << std::hex << (int)headerByte << std::dec
        << " 二进制: " << std::bitset<8>(headerByte) << std::endl;

    // 第1-4行：IP数据
    for (int i = 0; i < 4; i++) {
        FillMatrixRow(result.matrix[DATA_START_ROW + i], ipBytes[i]);
        std::cout << "数据行" << i + 1 << ": " << (int)ipBytes[i]
            << " 二进制: " << std::bitset<8>(ipBytes[i]) << std::endl;
    }

    result.isValid = true;
    return result;
}

// 解析IP地址
std::vector<uint8_t> ImprovedWatermarkManager::ParseIP(const std::string& ip)
{
    std::vector<uint8_t> bytes;
    std::istringstream iss(ip);
    std::string segment;

    while (std::getline(iss, segment, '.')) {
        try {
            int value = std::stoi(segment);
            if (value >= 0 && value <= 255) {
                bytes.push_back(static_cast<uint8_t>(value));
            }
            else {
                return {};
            }
        }
        catch (...) {
            return {};
        }
    }

    return bytes.size() == 4 ? bytes : std::vector<uint8_t>{};
}

// 填充矩阵行
void ImprovedWatermarkManager::FillMatrixRow(std::vector<bool>& row, uint8_t value)
{
    for (int i = 0; i < BITS_PER_ROW; i++) {
        row[i] = (value >> (7 - i)) & 1;
    }
}

// 创建改进的点阵图案
void ImprovedWatermarkManager::CreateImprovedDotMatrixPattern(HDC targetDC, int patternWidth, int patternHeight, const EncodedMatrix& encoded)
{
    if (!encoded.isValid) return;

    // 计算单个矩阵块的尺寸
    int blockWidth = BITS_PER_ROW * m_dotParams.dotSpacing;
    int blockHeight = TOTAL_ROWS * m_dotParams.dotSpacing;

    // 使用GDI+绘制高质量圆点
    Gdiplus::Graphics graphics(targetDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // 创建头部行画刷（红色）
    int headerR = GetRValue(m_dotParams.headerColor);
    int headerG = GetGValue(m_dotParams.headerColor);
    int headerB = GetBValue(m_dotParams.headerColor);
    int headerAlpha = static_cast<int>(m_dotParams.opacity * 255);
    Gdiplus::SolidBrush headerBrush(Gdiplus::Color(headerAlpha, headerR, headerG, headerB));

    // 创建数据行画刷（蓝色）
    int dataR = GetRValue(m_dotParams.dataColor);
    int dataG = GetGValue(m_dotParams.dataColor);
    int dataB = GetBValue(m_dotParams.dataColor);
    int dataAlpha = static_cast<int>(m_dotParams.opacity * 255);
    Gdiplus::SolidBrush dataBrush(Gdiplus::Color(dataAlpha, dataR, dataG, dataB));

    // 计算重复次数以覆盖整个区域
    int repeatX = (patternWidth / (blockWidth + m_dotParams.blockSpacing)) + 2;
    int repeatY = (patternHeight / (blockHeight + m_dotParams.blockSpacing)) + 2;

    // 绘制重复的点阵模式
    for (int ry = -1; ry < repeatY; ry++) {
        for (int rx = -1; rx < repeatX; rx++) {
            int baseX = rx * (blockWidth + m_dotParams.blockSpacing);
            int baseY = ry * (blockHeight + m_dotParams.blockSpacing);

            // 绘制当前块的所有位
            for (int row = 0; row < TOTAL_ROWS; row++) {
                for (int col = 0; col < BITS_PER_ROW; col++) {
                    if (encoded.matrix[row][col]) {
                        float x = static_cast<float>(baseX + col * m_dotParams.dotSpacing);
                        float y = static_cast<float>(baseY + row * m_dotParams.dotSpacing);
                        float diameter = static_cast<float>(m_dotParams.dotRadius * 2);

                        // 根据行类型选择颜色
                        Gdiplus::SolidBrush* brush = (row == HEADER_ROW) ? &headerBrush : &dataBrush;

                        // 绘制圆点
                        graphics.FillEllipse(brush,
                            x - m_dotParams.dotRadius,
                            y - m_dotParams.dotRadius,
                            diameter,
                            diameter);

                        // 如果启用调试信息，添加小标签
                        if (m_dotParams.showDebugInfo && rx == 0 && ry == 0) {
                            std::wstring label;
                            if (row == HEADER_ROW) {
                                label = L"H";
                            }
                            else {
                                label = L"D" + std::to_wstring(row);
                            }

                            Gdiplus::FontFamily fontFamily(L"Arial");
                            Gdiplus::Font font(&fontFamily, 8, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
                            Gdiplus::SolidBrush textBrush(Gdiplus::Color(200, 255, 255, 255));

                            graphics.DrawString(label.c_str(), label.length(), &font,
                                Gdiplus::PointF(x + 5, y - 10), &textBrush);
                        }
                    }
                }
            }
        }
    }

    // 如果启用调试信息，在左上角显示IP和校验信息
    if (m_dotParams.showDebugInfo) {
        Gdiplus::FontFamily fontFamily(L"Consolas");
        Gdiplus::Font font(&fontFamily, 12, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(180, 255, 255, 0));

        std::wstring debugInfo = L"IP: ";
        debugInfo += std::wstring(encoded.originalIP.begin(), encoded.originalIP.end());
        debugInfo += L" | Checksum: ";
        debugInfo += std::to_wstring(encoded.checksum);
        debugInfo += L" | Protocol: 1010xxxx";

        graphics.DrawString(debugInfo.c_str(), debugInfo.length(), &font,
            Gdiplus::PointF(10, 10), &textBrush);
    }
}

// 绘制多显示器改进点阵水印
void ImprovedWatermarkManager::DrawMultiMonitorImprovedDotWatermark()
{
    if (!m_currentMatrix.isValid) return;

    // 获取所有显示器信息
    std::vector<MonitorInfo> monitors = GetAllMonitors();
    if (monitors.empty()) return;

    // 计算所有显示器的总边界
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

    // 初始化位图为全透明
    memset(bits, 0, totalWidth * totalHeight * 4);

    // 计算水印画布大小（考虑旋转）
    int watermarkSize = static_cast<int>(sqrt(totalWidth * totalWidth + totalHeight * totalHeight) * 1.2);

    // 创建水印画布DC
    HDC watermarkDC = CreateCompatibleDC(screenDC);
    BITMAPINFO wmBmi = bmi;
    wmBmi.bmiHeader.biWidth = watermarkSize;
    wmBmi.bmiHeader.biHeight = -watermarkSize;

    void* wmBits = NULL;
    HBITMAP watermarkBitmap = CreateDIBSection(watermarkDC, &wmBmi, DIB_RGB_COLORS, &wmBits, NULL, 0);
    HBITMAP oldWatermarkBitmap = (HBITMAP)SelectObject(watermarkDC, watermarkBitmap);

    // 清空水印背景
    memset(wmBits, 0, watermarkSize * watermarkSize * 4);

    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 绘制改进的点阵图案到水印DC
    CreateImprovedDotMatrixPattern(watermarkDC, watermarkSize, watermarkSize, m_currentMatrix);

    // 创建位图对象用于旋转
    Gdiplus::Bitmap* srcBitmap = new Gdiplus::Bitmap(watermarkSize, watermarkSize,
        watermarkSize * 4, PixelFormat32bppPARGB, (BYTE*)wmBits);

    // 在主画布上绘制旋转的水印
    Gdiplus::Graphics memGraphics(memDC);
    memGraphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    memGraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // 计算旋转中心点（相对于总屏幕区域的中心）
    int centerX = totalWidth / 2;
    int centerY = totalHeight / 2;

    // 创建变换矩阵并旋转 - 模式时针旋转
    Gdiplus::Matrix matrix;
    matrix.RotateAt(-m_dotParams.rotation, Gdiplus::PointF((float)centerX, (float)centerY));
    memGraphics.SetTransform(&matrix);

    // 计算绘制位置
    int drawX = centerX - watermarkSize / 2;
    int drawY = centerY - watermarkSize / 2;

    // 如果正在闪烁，调整透明度
    if (m_isBlinking) {
        Gdiplus::ColorMatrix colorMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, (m_blinkCount % 2 == 0) ? 1.0f : 0.3f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
        };

        Gdiplus::ImageAttributes imageAttributes;
        imageAttributes.SetColorMatrix(&colorMatrix);

        memGraphics.DrawImage(srcBitmap,
            Gdiplus::Rect(drawX, drawY, watermarkSize, watermarkSize),
            0, 0, watermarkSize, watermarkSize,
            Gdiplus::UnitPixel, &imageAttributes);
    }
    else {
        // 绘制旋转后的水印
        memGraphics.DrawImage(srcBitmap, drawX, drawY);
    }

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
    SetWindowPos(m_hwnd, HWND_TOPMOST,
        totalBounds.left, totalBounds.top,
        totalWidth, totalHeight,
        SWP_NOACTIVATE);

    // 更新分层窗口
    UpdateLayeredWindow(
        m_hwnd,           // 目标窗口句柄
        screenDC,         // 屏幕DC
        &ptDst,           // 窗口位置
        &sizeWnd,         // 窗口大小
        memDC,            // 源DC
        &ptSrc,           // 源位置
        0,                // 颜色键（不使用）
        &blend,           // 混合函数
        ULW_ALPHA         // 使用Alpha通道
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

// 检查IP变化
void ImprovedWatermarkManager::CheckIPChange()
{
    std::string currentIP = GetIPAddress();

    if (m_dotParams.enableBlinking && currentIP != m_lastIP && !currentIP.empty()) {
        std::cout << "检测到IP变化: " << m_lastIP << " -> " << currentIP << std::endl;

        // 更新IP地址和编码矩阵
        m_dotParams.ipAddress = currentIP;
        m_currentMatrix = EncodeIPToImprovedProtocol(currentIP);
        m_lastIP = currentIP;

        // 启动闪烁
        StartBlinking();
    }
}

// 启动闪烁
void ImprovedWatermarkManager::StartBlinking()
{
    if (!m_isBlinking) {
        m_isBlinking = true;
        m_blinkCount = 0;
        SetTimer(m_hwnd, BLINK_TIMER_ID, 300, NULL);  // 每300ms闪烁一次

        // 设置闪烁持续时间
        SetTimer(m_hwnd, BLINK_TIMER_ID + 1, m_dotParams.blinkDuration, NULL);
    }
}

// 停止闪烁
void ImprovedWatermarkManager::StopBlinking()
{
    if (m_isBlinking) {
        m_isBlinking = false;
        KillTimer(m_hwnd, BLINK_TIMER_ID);
        KillTimer(m_hwnd, BLINK_TIMER_ID + 1);
        UpdateDotWatermark(); // 恢复正常显示
    }
}

// 更新点阵水印
void ImprovedWatermarkManager::UpdateDotWatermark()
{
    if (m_hwnd && m_running && m_isDotMode) {
        DrawMultiMonitorImprovedDotWatermark();
    }
}

// 更新水印（通用）
void ImprovedWatermarkManager::UpdateWatermark()
{
    if (m_hwnd && m_running) {
        if (m_isDotMode) {
            UpdateDotWatermark();
            // 检查IP变化
            CheckIPChange();
        }
        else {
            // 处理文本水印的变量替换等
            DrawMultiMonitorWatermark();
        }
    }
}

// 停止水印
void ImprovedWatermarkManager::StopWatermark()
{
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_ID);
        StopBlinking();
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    m_running = false;
    m_isDotMode = false;
}

// 静态窗口过程函数
LRESULT CALLBACK ImprovedWatermarkManager::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (s_pInstance) {
        return s_pInstance->WndProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 窗口过程函数
LRESULT CALLBACK ImprovedWatermarkManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID) {
            UpdateWatermark();
        }
        else if (wParam == BLINK_TIMER_ID) {
            // 闪烁定时器
            m_blinkCount++;
            UpdateDotWatermark();
        }
        else if (wParam == BLINK_TIMER_ID + 1) {
            // 停止闪烁定时器
            StopBlinking();
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

// 创建透明窗口
HWND ImprovedWatermarkManager::CreateTransparentWindow(int width, int height)
{
    // 注册窗口类
    const wchar_t* g_ClassName = L"ImprovedWatermarkWindow";

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
        L"Improved Watermark Window",
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
BOOL CALLBACK ImprovedWatermarkManager::EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
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
std::vector<MonitorInfo> ImprovedWatermarkManager::GetAllMonitors()
{
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

// 获取IP地址（简化版本，返回字符串）
std::string ImprovedWatermarkManager::GetIPAddress()
{
    // 使用 Windows Socket API 获取IP地址
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "0.0.0.0";
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        WSACleanup();
        return "0.0.0.0";
    }

    struct addrinfo hints, * res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        WSACleanup();
        return "0.0.0.0";
    }

    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);

    freeaddrinfo(res);
    WSACleanup();

    return std::string(ip);
}

// 设置改进的点阵水印参数
void ImprovedWatermarkManager::SetImprovedDotMatrixParams(const ImprovedDotMatrixParams& params)
{
    m_dotParams = params;

    // 重新编码IP地址
    if (!m_dotParams.ipAddress.empty()) {
        m_currentMatrix = EncodeIPToImprovedProtocol(m_dotParams.ipAddress);
        m_lastIP = m_dotParams.ipAddress;
    }

    // 如果正在运行，立即更新显示
    if (m_running && m_isDotMode) {
        UpdateDotWatermark();
    }
}

// 设置水印参数（兼容原有接口）
void ImprovedWatermarkManager::SetWatermarkParams(const WatermarkParams& params)
{
    m_params = params;

    // 处理文本中的变量
    if (m_params.text.find(L"%%") != std::wstring::npos) {
        m_params.text = ProcessTextVariables(m_params.text);
    }
}

// 开始普通水印（兼容原有接口）
bool ImprovedWatermarkManager::StartWatermark()
{
    if (!m_initialized || m_running) {
        return false;
    }

    m_isDotMode = false;

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

// 绘制多显示器水印（简化版本，主要为了兼容）
void ImprovedWatermarkManager::DrawMultiMonitorWatermark()
{
    // 这里可以保持原有的文本水印实现
    // 由于篇幅限制，这里提供一个简化的版本
    std::cout << "绘制普通文本水印: " << std::string(m_params.text.begin(), m_params.text.end()) << std::endl;
}

// 处理文本变量（简化版本）
std::wstring ImprovedWatermarkManager::ProcessTextVariables(const std::wstring& text)
{
    std::wstring result = text;

    // 处理时间变量
    if (result.find(L"%%datetime%%") != std::wstring::npos) {
        std::wstring currentTime = GetCurrentTimeString();
        size_t pos = 0;
        while ((pos = result.find(L"%%datetime%%", pos)) != std::wstring::npos) {
            result.replace(pos, 12, currentTime);
            pos += currentTime.length();
        }
    }

    // 处理IP变量
    if (result.find(L"%%ip%%") != std::wstring::npos) {
        std::string ip = GetIPAddress();
        std::wstring wip(ip.begin(), ip.end());
        size_t pos = 0;
        while ((pos = result.find(L"%%ip%%", pos)) != std::wstring::npos) {
            result.replace(pos, 6, wip);
            pos += wip.length();
        }
    }

    return result;
}

// 获取当前时间字符串
std::wstring ImprovedWatermarkManager::GetCurrentTimeString()
{
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);

    wchar_t timeStr[64];
    wcsftime(timeStr, sizeof(timeStr) / sizeof(wchar_t), L"%Y/%m/%d %H:%M", &localTime);

    return timeStr;
}

// 其他获取系统信息的函数（简化实现）
std::wstring ImprovedWatermarkManager::GetHostName()
{
    wchar_t hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname) / sizeof(wchar_t);
    if (GetComputerNameW(hostname, &size)) {
        return hostname;
    }
    return L"Unknown";
}

std::wstring ImprovedWatermarkManager::GetMACAddress()
{
    // 简化实现，实际应用中可以参考原有代码
    return L"00:00:00:00:00:00";
}

std::wstring ImprovedWatermarkManager::GetLoginName()
{
    wchar_t username[UNLEN + 1];
    DWORD size = sizeof(username) / sizeof(wchar_t);
    if (GetUserNameW(username, &size)) {
        return username;
    }
    return L"Unknown";
}

// 使用示例
/*
int main()
{
    ImprovedWatermarkManager watermark;

    // 初始化
    if (!watermark.Initialize(GetModuleHandle(NULL))) {
        std::cout << "初始化失败" << std::endl;
        return -1;
    }

    // 配置改进的点阵水印参数
    ImprovedDotMatrixParams dotParams;
    dotParams.ipAddress = "192.168.1.100";
    dotParams.dotRadius = 3;
    dotParams.dotSpacing = 18;
    dotParams.blockSpacing = 120;
    dotParams.headerColor = RGB(255, 0, 0);      // 红色头部行
    dotParams.dataColor = RGB(0, 100, 255);      // 蓝色数据行
    dotParams.opacity = 0.7f;
    dotParams.rotation = 15.0f;
    dotParams.hSpacing = 150;
    dotParams.vSpacing = 120;
    dotParams.showDebugInfo = true;              // 显示调试信息
    dotParams.enableBlinking = true;             // 启用IP变化闪烁
    dotParams.blinkDuration = 3000;              // 闪烁3秒

    watermark.SetImprovedDotMatrixParams(dotParams);

    // 启动改进的点阵水印
    if (watermark.StartImprovedDotWatermark()) {
        std::cout << "改进点阵水印启动成功！" << std::endl;
        std::cout << "特性:" << std::endl;
        std::cout << "- 红色头部行（同步+校验）" << std::endl;
        std::cout << "- 蓝色数据行（IP地址）" << std::endl;
        std::cout << "- IP变化时自动闪烁提醒" << std::endl;
        std::cout << "- 支持调试信息显示" << std::endl;

        // 消息循环
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        std::cout << "启动水印失败" << std::endl;
    }

    return 0;
}
*/