// �Ľ���ˮӡϵͳ - ֧���µĵ���Э��


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

// ��ʼ����̬��Ա
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

    // ��ʼ���Ľ��ĵ���ˮӡĬ�ϲ���
    m_dotParams.ipAddress = "192.168.1.100";
    m_dotParams.dotRadius = 3;
    m_dotParams.dotSpacing = 15;
    m_dotParams.blockSpacing = 100;
    m_dotParams.headerColor = RGB(255, 0, 0);      // ��ɫͷ����
    m_dotParams.dataColor = RGB(0, 100, 255);      // ��ɫ������
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

    // ��ʼ��GDI+
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

    // ����IP��ַΪ�Ľ�Э�����
    m_currentMatrix = EncodeIPToImprovedProtocol(m_dotParams.ipAddress);
    if (!m_currentMatrix.isValid) {
        std::cout << "IP��ַ����ʧ��: " << m_dotParams.ipAddress << std::endl;
        return false;
    }

    // ��¼��ǰIP���ڱ仯���
    m_lastIP = m_dotParams.ipAddress;

    // ��������
    m_hwnd = CreateTransparentWindow(
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN)
    );

    if (!m_hwnd) {
        return false;
    }

    // ���ö�ʱ�������ڶ��ڸ���ˮӡ�ͼ��IP�仯
    SetTimer(m_hwnd, TIMER_ID, 3000, NULL);  // ÿ3����һ��IP�仯

    // ��������һ�ε���ˮӡ
    UpdateDotWatermark();

    m_running = true;

    std::cout << "�Ľ�����ˮӡ�����ɹ�" << std::endl;
    std::cout << "IP: " << m_currentMatrix.originalIP << std::endl;
    std::cout << "У���: " << (int)m_currentMatrix.checksum << std::endl;

    return true;
}

// IP����Ϊ�Ľ�Э��
ImprovedWatermarkManager::EncodedMatrix ImprovedWatermarkManager::EncodeIPToImprovedProtocol(const std::string& ip)
{
    EncodedMatrix result;
    result.originalIP = ip;

    // ����IP��ַ
    std::vector<uint8_t> ipBytes = ParseIP(ip);
    if (ipBytes.size() != 4) {
        std::cout << "��Ч��IP��ַ��ʽ: " << ip << std::endl;
        return result;
    }

    // ����У��ͣ�4λ��
    uint32_t sum = 0;
    for (uint8_t byte : ipBytes) {
        sum += byte;
    }
    uint8_t checksum = sum & 0x0F; // ֻȡ��4λ
    result.checksum = checksum;

    std::cout << "\n=== ����IP��ַ ===" << std::endl;
    std::cout << "IP: " << ip << std::endl;
    std::cout << "�ֽ�: ";
    for (int i = 0; i < 4; i++) {
        std::cout << (int)ipBytes[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "У���: " << (int)checksum << " (0x" << std::hex << (int)checksum << std::dec << ")" << std::endl;

    // ��0�У�����ͷ�� [ͬ��λ1010][У���4bit]
    uint8_t headerByte = SYNC_PATTERN | checksum;
    FillMatrixRow(result.matrix[HEADER_ROW], headerByte);

    std::cout << "ͷ���ֽ�: 0x" << std::hex << (int)headerByte << std::dec
        << " ������: " << std::bitset<8>(headerByte) << std::endl;

    // ��1-4�У�IP����
    for (int i = 0; i < 4; i++) {
        FillMatrixRow(result.matrix[DATA_START_ROW + i], ipBytes[i]);
        std::cout << "������" << i + 1 << ": " << (int)ipBytes[i]
            << " ������: " << std::bitset<8>(ipBytes[i]) << std::endl;
    }

    result.isValid = true;
    return result;
}

// ����IP��ַ
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

// ��������
void ImprovedWatermarkManager::FillMatrixRow(std::vector<bool>& row, uint8_t value)
{
    for (int i = 0; i < BITS_PER_ROW; i++) {
        row[i] = (value >> (7 - i)) & 1;
    }
}

// �����Ľ��ĵ���ͼ��
void ImprovedWatermarkManager::CreateImprovedDotMatrixPattern(HDC targetDC, int patternWidth, int patternHeight, const EncodedMatrix& encoded)
{
    if (!encoded.isValid) return;

    // ���㵥�������ĳߴ�
    int blockWidth = BITS_PER_ROW * m_dotParams.dotSpacing;
    int blockHeight = TOTAL_ROWS * m_dotParams.dotSpacing;

    // ʹ��GDI+���Ƹ�����Բ��
    Gdiplus::Graphics graphics(targetDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // ����ͷ���л�ˢ����ɫ��
    int headerR = GetRValue(m_dotParams.headerColor);
    int headerG = GetGValue(m_dotParams.headerColor);
    int headerB = GetBValue(m_dotParams.headerColor);
    int headerAlpha = static_cast<int>(m_dotParams.opacity * 255);
    Gdiplus::SolidBrush headerBrush(Gdiplus::Color(headerAlpha, headerR, headerG, headerB));

    // ���������л�ˢ����ɫ��
    int dataR = GetRValue(m_dotParams.dataColor);
    int dataG = GetGValue(m_dotParams.dataColor);
    int dataB = GetBValue(m_dotParams.dataColor);
    int dataAlpha = static_cast<int>(m_dotParams.opacity * 255);
    Gdiplus::SolidBrush dataBrush(Gdiplus::Color(dataAlpha, dataR, dataG, dataB));

    // �����ظ������Ը�����������
    int repeatX = (patternWidth / (blockWidth + m_dotParams.blockSpacing)) + 2;
    int repeatY = (patternHeight / (blockHeight + m_dotParams.blockSpacing)) + 2;

    // �����ظ��ĵ���ģʽ
    for (int ry = -1; ry < repeatY; ry++) {
        for (int rx = -1; rx < repeatX; rx++) {
            int baseX = rx * (blockWidth + m_dotParams.blockSpacing);
            int baseY = ry * (blockHeight + m_dotParams.blockSpacing);

            // ���Ƶ�ǰ�������λ
            for (int row = 0; row < TOTAL_ROWS; row++) {
                for (int col = 0; col < BITS_PER_ROW; col++) {
                    if (encoded.matrix[row][col]) {
                        float x = static_cast<float>(baseX + col * m_dotParams.dotSpacing);
                        float y = static_cast<float>(baseY + row * m_dotParams.dotSpacing);
                        float diameter = static_cast<float>(m_dotParams.dotRadius * 2);

                        // ����������ѡ����ɫ
                        Gdiplus::SolidBrush* brush = (row == HEADER_ROW) ? &headerBrush : &dataBrush;

                        // ����Բ��
                        graphics.FillEllipse(brush,
                            x - m_dotParams.dotRadius,
                            y - m_dotParams.dotRadius,
                            diameter,
                            diameter);

                        // ������õ�����Ϣ�����С��ǩ
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

    // ������õ�����Ϣ�������Ͻ���ʾIP��У����Ϣ
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

// ���ƶ���ʾ���Ľ�����ˮӡ
void ImprovedWatermarkManager::DrawMultiMonitorImprovedDotWatermark()
{
    if (!m_currentMatrix.isValid) return;

    // ��ȡ������ʾ����Ϣ
    std::vector<MonitorInfo> monitors = GetAllMonitors();
    if (monitors.empty()) return;

    // ����������ʾ�����ܱ߽�
    RECT totalBounds = monitors[0].rect;
    for (size_t i = 1; i < monitors.size(); i++) {
        totalBounds.left = min(totalBounds.left, monitors[i].rect.left);
        totalBounds.top = min(totalBounds.top, monitors[i].rect.top);
        totalBounds.right = max(totalBounds.right, monitors[i].rect.right);
        totalBounds.bottom = max(totalBounds.bottom, monitors[i].rect.bottom);
    }

    // ���㸲��������ʾ���Ĵ�С
    int totalWidth = totalBounds.right - totalBounds.left;
    int totalHeight = totalBounds.bottom - totalBounds.top;

    // ������ĻDC
    HDC screenDC = GetDC(NULL);

    // ��������DC��32λARGBλͼ
    HDC memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = totalWidth;
    bmi.bmiHeader.biHeight = -totalHeight; // ���϶���
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;    // 32λARGB
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = NULL;
    HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    // ��ʼ��λͼΪȫ͸��
    memset(bits, 0, totalWidth * totalHeight * 4);

    // ����ˮӡ������С��������ת��
    int watermarkSize = static_cast<int>(sqrt(totalWidth * totalWidth + totalHeight * totalHeight) * 1.2);

    // ����ˮӡ����DC
    HDC watermarkDC = CreateCompatibleDC(screenDC);
    BITMAPINFO wmBmi = bmi;
    wmBmi.bmiHeader.biWidth = watermarkSize;
    wmBmi.bmiHeader.biHeight = -watermarkSize;

    void* wmBits = NULL;
    HBITMAP watermarkBitmap = CreateDIBSection(watermarkDC, &wmBmi, DIB_RGB_COLORS, &wmBits, NULL, 0);
    HBITMAP oldWatermarkBitmap = (HBITMAP)SelectObject(watermarkDC, watermarkBitmap);

    // ���ˮӡ����
    memset(wmBits, 0, watermarkSize * watermarkSize * 4);

    // ��ʼ��GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // ���ƸĽ��ĵ���ͼ����ˮӡDC
    CreateImprovedDotMatrixPattern(watermarkDC, watermarkSize, watermarkSize, m_currentMatrix);

    // ����λͼ����������ת
    Gdiplus::Bitmap* srcBitmap = new Gdiplus::Bitmap(watermarkSize, watermarkSize,
        watermarkSize * 4, PixelFormat32bppPARGB, (BYTE*)wmBits);

    // ���������ϻ�����ת��ˮӡ
    Gdiplus::Graphics memGraphics(memDC);
    memGraphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    memGraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // ������ת���ĵ㣨���������Ļ��������ģ�
    int centerX = totalWidth / 2;
    int centerY = totalHeight / 2;

    // �����任������ת - ģʽʱ����ת
    Gdiplus::Matrix matrix;
    matrix.RotateAt(-m_dotParams.rotation, Gdiplus::PointF((float)centerX, (float)centerY));
    memGraphics.SetTransform(&matrix);

    // �������λ��
    int drawX = centerX - watermarkSize / 2;
    int drawY = centerY - watermarkSize / 2;

    // ���������˸������͸����
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
        // ������ת���ˮӡ
        memGraphics.DrawImage(srcBitmap, drawX, drawY);
    }

    // ��ȡ����λ��
    POINT ptDst = { totalBounds.left, totalBounds.top };

    // ���÷ֲ㴰�ڻ�ϲ��� 
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;     // ʹ��ÿ����Alphaֵ
    blend.AlphaFormat = AC_SRC_ALPHA;    // ʹ��Ԥ��Alpha

    SIZE sizeWnd = { totalWidth, totalHeight };
    POINT ptSrc = { 0, 0 };

    // �������ڴ�С�Ը���������ʾ��
    SetWindowPos(m_hwnd, HWND_TOPMOST,
        totalBounds.left, totalBounds.top,
        totalWidth, totalHeight,
        SWP_NOACTIVATE);

    // ���·ֲ㴰��
    UpdateLayeredWindow(
        m_hwnd,           // Ŀ�괰�ھ��
        screenDC,         // ��ĻDC
        &ptDst,           // ����λ��
        &sizeWnd,         // ���ڴ�С
        memDC,            // ԴDC
        &ptSrc,           // Դλ��
        0,                // ��ɫ������ʹ�ã�
        &blend,           // ��Ϻ���
        ULW_ALPHA         // ʹ��Alphaͨ��
    );

    // ������Դ
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

// ���IP�仯
void ImprovedWatermarkManager::CheckIPChange()
{
    std::string currentIP = GetIPAddress();

    if (m_dotParams.enableBlinking && currentIP != m_lastIP && !currentIP.empty()) {
        std::cout << "��⵽IP�仯: " << m_lastIP << " -> " << currentIP << std::endl;

        // ����IP��ַ�ͱ������
        m_dotParams.ipAddress = currentIP;
        m_currentMatrix = EncodeIPToImprovedProtocol(currentIP);
        m_lastIP = currentIP;

        // ������˸
        StartBlinking();
    }
}

// ������˸
void ImprovedWatermarkManager::StartBlinking()
{
    if (!m_isBlinking) {
        m_isBlinking = true;
        m_blinkCount = 0;
        SetTimer(m_hwnd, BLINK_TIMER_ID, 300, NULL);  // ÿ300ms��˸һ��

        // ������˸����ʱ��
        SetTimer(m_hwnd, BLINK_TIMER_ID + 1, m_dotParams.blinkDuration, NULL);
    }
}

// ֹͣ��˸
void ImprovedWatermarkManager::StopBlinking()
{
    if (m_isBlinking) {
        m_isBlinking = false;
        KillTimer(m_hwnd, BLINK_TIMER_ID);
        KillTimer(m_hwnd, BLINK_TIMER_ID + 1);
        UpdateDotWatermark(); // �ָ�������ʾ
    }
}

// ���µ���ˮӡ
void ImprovedWatermarkManager::UpdateDotWatermark()
{
    if (m_hwnd && m_running && m_isDotMode) {
        DrawMultiMonitorImprovedDotWatermark();
    }
}

// ����ˮӡ��ͨ�ã�
void ImprovedWatermarkManager::UpdateWatermark()
{
    if (m_hwnd && m_running) {
        if (m_isDotMode) {
            UpdateDotWatermark();
            // ���IP�仯
            CheckIPChange();
        }
        else {
            // �����ı�ˮӡ�ı����滻��
            DrawMultiMonitorWatermark();
        }
    }
}

// ֹͣˮӡ
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

// ��̬���ڹ��̺���
LRESULT CALLBACK ImprovedWatermarkManager::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (s_pInstance) {
        return s_pInstance->WndProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ���ڹ��̺���
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
            // ��˸��ʱ��
            m_blinkCount++;
            UpdateDotWatermark();
        }
        else if (wParam == BLINK_TIMER_ID + 1) {
            // ֹͣ��˸��ʱ��
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

// ����͸������
HWND ImprovedWatermarkManager::CreateTransparentWindow(int width, int height)
{
    // ע�ᴰ����
    const wchar_t* g_ClassName = L"ImprovedWatermarkWindow";

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = g_ClassName;
    RegisterClassEx(&wc);

    // ��������
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

    // ���ô�����ʽ
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE)
        & ~WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);

    // ��ʾ����
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

// ö����ʾ���ص�����
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

// ��ȡ������ʾ����Ϣ
std::vector<MonitorInfo> ImprovedWatermarkManager::GetAllMonitors()
{
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

// ��ȡIP��ַ���򻯰汾�������ַ�����
std::string ImprovedWatermarkManager::GetIPAddress()
{
    // ʹ�� Windows Socket API ��ȡIP��ַ
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

// ���øĽ��ĵ���ˮӡ����
void ImprovedWatermarkManager::SetImprovedDotMatrixParams(const ImprovedDotMatrixParams& params)
{
    m_dotParams = params;

    // ���±���IP��ַ
    if (!m_dotParams.ipAddress.empty()) {
        m_currentMatrix = EncodeIPToImprovedProtocol(m_dotParams.ipAddress);
        m_lastIP = m_dotParams.ipAddress;
    }

    // ����������У�����������ʾ
    if (m_running && m_isDotMode) {
        UpdateDotWatermark();
    }
}

// ����ˮӡ����������ԭ�нӿڣ�
void ImprovedWatermarkManager::SetWatermarkParams(const WatermarkParams& params)
{
    m_params = params;

    // �����ı��еı���
    if (m_params.text.find(L"%%") != std::wstring::npos) {
        m_params.text = ProcessTextVariables(m_params.text);
    }
}

// ��ʼ��ͨˮӡ������ԭ�нӿڣ�
bool ImprovedWatermarkManager::StartWatermark()
{
    if (!m_initialized || m_running) {
        return false;
    }

    m_isDotMode = false;

    // ��������
    m_hwnd = CreateTransparentWindow(
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN)
    );

    if (!m_hwnd) {
        return false;
    }

    // ���ö�ʱ�������ڸ���ˮӡ
    SetTimer(m_hwnd, TIMER_ID, 1000, NULL);

    // ��������һ��ˮӡ
    UpdateWatermark();

    m_running = true;
    return true;
}

// ���ƶ���ʾ��ˮӡ���򻯰汾����ҪΪ�˼��ݣ�
void ImprovedWatermarkManager::DrawMultiMonitorWatermark()
{
    // ������Ա���ԭ�е��ı�ˮӡʵ��
    // ����ƪ�����ƣ������ṩһ���򻯵İ汾
    std::cout << "������ͨ�ı�ˮӡ: " << std::string(m_params.text.begin(), m_params.text.end()) << std::endl;
}

// �����ı��������򻯰汾��
std::wstring ImprovedWatermarkManager::ProcessTextVariables(const std::wstring& text)
{
    std::wstring result = text;

    // ����ʱ�����
    if (result.find(L"%%datetime%%") != std::wstring::npos) {
        std::wstring currentTime = GetCurrentTimeString();
        size_t pos = 0;
        while ((pos = result.find(L"%%datetime%%", pos)) != std::wstring::npos) {
            result.replace(pos, 12, currentTime);
            pos += currentTime.length();
        }
    }

    // ����IP����
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

// ��ȡ��ǰʱ���ַ���
std::wstring ImprovedWatermarkManager::GetCurrentTimeString()
{
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);

    wchar_t timeStr[64];
    wcsftime(timeStr, sizeof(timeStr) / sizeof(wchar_t), L"%Y/%m/%d %H:%M", &localTime);

    return timeStr;
}

// ������ȡϵͳ��Ϣ�ĺ�������ʵ�֣�
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
    // ��ʵ�֣�ʵ��Ӧ���п��Բο�ԭ�д���
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

// ʹ��ʾ��
/*
int main()
{
    ImprovedWatermarkManager watermark;

    // ��ʼ��
    if (!watermark.Initialize(GetModuleHandle(NULL))) {
        std::cout << "��ʼ��ʧ��" << std::endl;
        return -1;
    }

    // ���øĽ��ĵ���ˮӡ����
    ImprovedDotMatrixParams dotParams;
    dotParams.ipAddress = "192.168.1.100";
    dotParams.dotRadius = 3;
    dotParams.dotSpacing = 18;
    dotParams.blockSpacing = 120;
    dotParams.headerColor = RGB(255, 0, 0);      // ��ɫͷ����
    dotParams.dataColor = RGB(0, 100, 255);      // ��ɫ������
    dotParams.opacity = 0.7f;
    dotParams.rotation = 15.0f;
    dotParams.hSpacing = 150;
    dotParams.vSpacing = 120;
    dotParams.showDebugInfo = true;              // ��ʾ������Ϣ
    dotParams.enableBlinking = true;             // ����IP�仯��˸
    dotParams.blinkDuration = 3000;              // ��˸3��

    watermark.SetImprovedDotMatrixParams(dotParams);

    // �����Ľ��ĵ���ˮӡ
    if (watermark.StartImprovedDotWatermark()) {
        std::cout << "�Ľ�����ˮӡ�����ɹ���" << std::endl;
        std::cout << "����:" << std::endl;
        std::cout << "- ��ɫͷ���У�ͬ��+У�飩" << std::endl;
        std::cout << "- ��ɫ�����У�IP��ַ��" << std::endl;
        std::cout << "- IP�仯ʱ�Զ���˸����" << std::endl;
        std::cout << "- ֧�ֵ�����Ϣ��ʾ" << std::endl;

        // ��Ϣѭ��
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        std::cout << "����ˮӡʧ��" << std::endl;
    }

    return 0;
}
*/