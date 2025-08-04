// ���Ȱ��� winsock2.h
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>  // windows.h ������ winsock2.h ֮��
#include <iphlpapi.h>
#include <lmcons.h>

// Ȼ����� GDI+ ������ͷ�ļ�
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

// ��ʼ����̬��Ա
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

    // ��ʼ��GDI+
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

void WatermarkManager::StopWatermark()
{
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_ID);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    m_running = false;
}

// �� UpdateWatermark �����У�����ı��������¹��� - ֻ����ʱ�����
void WatermarkManager::UpdateWatermark()
{
    if (m_hwnd && m_running) {
        // ����ı����Ƿ����ʱ�����������������
        if (m_params.text.find(L"%%datetime%%") != std::wstring::npos) {
            // ������ʱ�ı����������
            std::wstring tempText = m_params.text;
            std::wstring::size_type pos = 0;

            // ���滻ʱ���������Ϊ��������һ�㲻��仯
            while ((pos = tempText.find(L"%%datetime%%", pos)) != std::wstring::npos) {
                tempText.replace(pos, 12, GetCurrentTimeString());
                pos += GetCurrentTimeString().length();
            }

            // ��ʱ���洦�����ı�
            std::wstring originalText = m_params.text;
            m_params.text = tempText;

            // ����ˮӡ
            DrawMultiMonitorWatermark();

            // �ָ�ԭʼ�ı��������������Ա��´θ���
            m_params.text = originalText;
        }
        else {
            // û��ʱ�������ֱ�ӻ���
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
    // ע�ᴰ����
    const wchar_t* g_ClassName = L"WatermarkWindow";

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
        L"Watermark Window",
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

// ��ȡ������ʾ����Ϣ
std::vector<MonitorInfo> WatermarkManager::GetAllMonitors()
{
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

// �޸ĺ��ˮӡ���ƺ�����֧�ֶ���ʾ��
void WatermarkManager::DrawMultiMonitorWatermark()
{
    // ��ȡ������ʾ����Ϣ
    std::vector<MonitorInfo> monitors = GetAllMonitors();
    if (monitors.empty()) return;

    // ����������ʾ��������߽�
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

    // ��ʼ��λͼΪȫ͸�� (ARGB = 0x00000000)
    memset(bits, 0, totalWidth * totalHeight * 4);

    // ����ˮӡͼ���С
    int watermarkSize = (int)(sqrt(totalWidth * totalWidth + totalHeight * totalHeight) * 1.2);

    // ����ˮӡDC��λͼ
    HDC watermarkDC = CreateCompatibleDC(screenDC);
    BITMAPINFO wmBmi = bmi;
    wmBmi.bmiHeader.biWidth = watermarkSize;
    wmBmi.bmiHeader.biHeight = -watermarkSize;

    void* wmBits = NULL;
    HBITMAP watermarkBitmap = CreateDIBSection(watermarkDC, &wmBmi, DIB_RGB_COLORS, &wmBits, NULL, 0);
    HBITMAP oldWatermarkBitmap = (HBITMAP)SelectObject(watermarkDC, watermarkBitmap);

    // ��ʼ��ˮӡλͼΪȫ͸��
    memset(wmBits, 0, watermarkSize * watermarkSize * 4);

    // ��ʼ��GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // ��COLORREF��ȡRGBֵ
    int r = GetRValue(m_params.textColor);
    int g = GetGValue(m_params.textColor);
    int b = GetBValue(m_params.textColor);

    // ʹ��GDI+ֱ�ӻ��ƴ�Alpha��ָ����ɫ���ı�
    Gdiplus::Graphics watermarkGraphics(watermarkDC);

    // ���ø������ı���Ⱦ
    watermarkGraphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    watermarkGraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // �����ı���ɫ��͸����ѡ����ʵ���Ⱦ��ʽ
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

    // Ϊ�˴����͸�������������һ����СAlphaֵ
    int actualAlpha = m_params.textAlpha;
    if (actualAlpha < 30) actualAlpha = 30; // ȷ��������һ��ɼ�

    // ʹ�ô������ɫ��͸����ֵ
    Gdiplus::SolidBrush brush(Gdiplus::Color(actualAlpha, r, g, b));

    // �����ı��ߴ� (ʹ��GDI+)
    Gdiplus::RectF layoutRect;
    Gdiplus::RectF boundingBox;
    watermarkGraphics.MeasureString(m_params.text.c_str(), m_params.text.length(), &font, Gdiplus::PointF(0, 0), &boundingBox);

    int textWidth = (int)boundingBox.Width;
    int textHeight = (int)boundingBox.Height;

    // ��ˮӡDC�ϻ�������״�ı���ʹ�õ�����ˮƽ�ʹ�ֱ���
    for (int y = -textHeight; y < watermarkSize + textHeight; y += textHeight + m_params.vSpacing) {
        for (int x = -textWidth; x < watermarkSize + textWidth; x += textWidth + m_params.hSpacing) {
            watermarkGraphics.DrawString(m_params.text.c_str(), m_params.text.length(), &font, Gdiplus::PointF((float)x, (float)y), &brush);
        }
    }

    // ֻ�Ը�͸���ȵİ�ɫ�ı��������ȹ���
    if (r > 250 && g > 250 && b > 250 && m_params.textAlpha > 100) {
        BYTE* pBits = (BYTE*)wmBits;
        for (int y = 0; y < watermarkSize; y++) {
            for (int x = 0; x < watermarkSize; x++) {
                int index = (y * watermarkSize + x) * 4;
                // RGB ����
                BYTE pixelB = pBits[index];
                BYTE pixelG = pBits[index + 1];
                BYTE pixelR = pBits[index + 2];
                BYTE pixelA = pBits[index + 3];

                // ������������ (�򻯼�Ȩ)
                int brightness = (pixelR + pixelG + pixelB) / 3;

                // ֻ�԰���Ե���д���
                if (brightness < 180 && pixelA < 80) {
                    pBits[index + 3] = 0; // ����Ϊ͸��
                }
            }
        }
    }

    // ����GDI+���������ת����
    Gdiplus::Bitmap* srcBitmap = new Gdiplus::Bitmap(watermarkSize, watermarkSize, watermarkSize * 4,
        PixelFormat32bppPARGB, (BYTE*)wmBits); // ʹ��Ԥ��Alpha

    Gdiplus::Graphics memGraphics(memDC);

    // ���ø�������ͼģʽ
    memGraphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    memGraphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    // ʹ�ú��ı�������ͬ����Ⱦ����
    if (m_params.textAlpha < 100) {
        memGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    }
    else if (r > 250 && g > 250 && b > 250) {
        memGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
    }
    else {
        memGraphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    }

    // ������ת���ģ����������������Ļ�����ģ�
    int centerX = totalWidth / 2;
    int centerY = totalHeight / 2;

    // �����任������ת - ��ʱ����ת
    Gdiplus::Matrix matrix;
    matrix.RotateAt(-m_params.angle, Gdiplus::PointF((float)centerX, (float)centerY));
    memGraphics.SetTransform(&matrix);

    // �������λ��
    int drawX = centerX - watermarkSize / 2;
    int drawY = centerY - watermarkSize / 2;

    // ������ת���ˮӡ
    memGraphics.DrawImage(srcBitmap, drawX, drawY);

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
    SetWindowPos(m_hwnd, NULL, totalBounds.left, totalBounds.top,
        totalWidth, totalHeight, SWP_NOZORDER | SWP_NOACTIVATE);

    // ���·ֲ㴰��
    UpdateLayeredWindow(
        m_hwnd,           // Ŀ�괰�ھ��
        screenDC,       // ��ĻDC
        &ptDst,         // ����λ��
        &sizeWnd,       // ���ڴ�С
        memDC,          // ԴDC
        &ptSrc,         // Դλ��
        0,              // ��ɫ������ʹ�ã�
        &blend,         // ��Ϻ���
        ULW_ALPHA       // ʹ��Alphaͨ��
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

void WatermarkManager::SetWatermarkParams(const WatermarkParams& params)
{
    // �ȸ��Ʋ���
    m_params = params;

    // �����ı��еı���������еĻ���
    if (m_params.text.find(L"%%") != std::wstring::npos) {
        m_params.text = ProcessTextVariables(m_params.text);
    }
}

// �����ı�����
std::wstring WatermarkManager::ProcessTextVariables(const std::wstring& text)
{
    std::wstring result = text;
    std::wstring::size_type pos = 0;

    // �滻ʱ�����
    while ((pos = result.find(L"%%datetime%%", pos)) != std::wstring::npos) {
        result.replace(pos, 12, GetCurrentTimeString());
        pos += GetCurrentTimeString().length();
    }

    // �滻����������
    pos = 0;
    while ((pos = result.find(L"%%hostname%%", pos)) != std::wstring::npos) {
        std::wstring hostname = GetHostName();
        result.replace(pos, 12, hostname);
        pos += hostname.length();
    }

    // �滻IP��ַ����
    pos = 0;
    while ((pos = result.find(L"%%ip%%", pos)) != std::wstring::npos) {
        std::wstring ip = GetIPAddress();
        result.replace(pos, 6, ip);
        pos += ip.length();
    }

    // �滻MAC��ַ����
    pos = 0;
    while ((pos = result.find(L"%%mac%%", pos)) != std::wstring::npos) {
        std::wstring mac = GetMACAddress();
        result.replace(pos, 7, mac);
        pos += mac.length();
    }

    // �滻��¼������
    pos = 0;
    while ((pos = result.find(L"%%loginname%%", pos)) != std::wstring::npos) {
        std::wstring loginname = GetLoginName();
        result.replace(pos, 13, loginname);
        pos += loginname.length();
    }

    return result;
}

// ��ȡ��ǰʱ���ַ���
std::wstring WatermarkManager::GetCurrentTimeString()
{
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);

    wchar_t timeStr[64];
    wcsftime(timeStr, sizeof(timeStr) / sizeof(wchar_t), L"%Y/%m/%d %H:%M", &localTime);

    return timeStr;
}

// ��ȡ������
std::wstring WatermarkManager::GetHostName()
{
    wchar_t hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname) / sizeof(wchar_t);
    if (GetComputerNameW(hostname, &size)) {
        return hostname;
    }
    return L"Unknown";
}

// ��ȡIP��ַ
std::wstring WatermarkManager::GetIPAddress()
{
    // ʹ�� Windows Socket API ��ȡIP��ַ
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

    // ת��Ϊ���ַ�
    size_t size = strlen(ip) + 1;
    wchar_t* w_ip = new wchar_t[size];
    size_t outSize;
    mbstowcs_s(&outSize, w_ip, size, ip, size - 1);
    std::wstring result(w_ip);
    delete[] w_ip;

    return result;
}

// ��ȡMAC��ַ
std::wstring WatermarkManager::GetMACAddress()
{
    // ���Ȼ�ȡIP��ַ
    std::wstring ipAddress = GetIPAddress();
    if (ipAddress == L"0.0.0.0") {
        return L"00:00:00:00:00:00";
    }

    // ʹ��IP��ַ��ص���������������ȡMAC
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
            // ת��Ϊ���ַ�
            size_t size = strlen(pAdapter->IpAddressList.IpAddress.String) + 1;
            wchar_t* w_ip = new wchar_t[size];
            size_t outSize;
            mbstowcs_s(&outSize, w_ip, size, pAdapter->IpAddressList.IpAddress.String, size - 1);
            std::wstring adapterIp(w_ip);
            delete[] w_ip;

            if (adapterIp == ipAddress) {
                // �ҵ�ƥ�������������ȡMAC��ַ
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

// ��ȡ��ǰ��¼�û���
std::wstring WatermarkManager::GetLoginName()
{
    wchar_t username[UNLEN + 1];
    DWORD size = sizeof(username) / sizeof(wchar_t);
    if (GetUserNameW(username, &size)) {
        return username;
    }
    return L"Unknown";
}
