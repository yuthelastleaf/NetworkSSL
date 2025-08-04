#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <GdiPlus.h>
// #include <opencv2/opencv.hpp>

// �����ṹ�壬���ڴ洢��ʾ����Ϣ
struct MonitorInfo {
    HMONITOR hMonitor;
    RECT rect;          // ��ʾ������������Ļ���꣩
    bool isPrimary;     // �Ƿ�Ϊ����ʾ��
};

// ˮӡ�����ṹ��
struct WatermarkParams {
    std::wstring text;
    int angle;
    int hSpacing;
    int vSpacing;
    std::wstring fontName;
    int fontSize;
    int textAlpha;
    COLORREF textColor;
};

class WatermarkManager {
public:
    WatermarkManager();
    ~WatermarkManager();

    // ��ʼ��
    bool Initialize(HINSTANCE hInstance);

    // ����ˮӡ����
    void SetWatermarkParams(const WatermarkParams& params);

    // ��ʼ��ʾˮӡ
    bool StartWatermark();

    // ֹͣ��ʾˮӡ
    void StopWatermark();

    // ����ˮӡ
    void UpdateWatermark();

    // ��̬���ڹ��̺���
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // ���ڹ��̺���
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // ����͸������
    HWND CreateTransparentWindow(int width, int height);

    // ��ȡ������ʾ����Ϣ
    std::vector<MonitorInfo> GetAllMonitors();

    // ö����ʾ���ص�����
    static BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

    // ���ƶ���ʾ��ˮӡ
    void DrawMultiMonitorWatermark();

    
private:
    // �����ı��еı���
    std::wstring ProcessTextVariables(const std::wstring& text);

    // ��ȡ��ǰʱ���ַ��� (��ʽ: YYYY/MM/DD HH:MM)
    std::wstring GetCurrentTimeString();

    // ��ȡ������
    std::wstring GetHostName();

    // ��ȡIP��ַ
    std::wstring GetIPAddress();

    // ��ȡMAC��ַ
    std::wstring GetMACAddress();

    // ��ȡ��ǰ��¼�û���
    std::wstring GetLoginName();

private:
    HWND m_hwnd;                     // ���ھ��
    HINSTANCE m_hInstance;           // ����ʵ�����
    WatermarkParams m_params;        // ˮӡ����
    const UINT_PTR TIMER_ID = 1;     // ��ʱ��ID
    ULONG_PTR m_gdiplusToken;        // GDI+����
    bool m_initialized;              // ��ʼ����־
    bool m_running;                  // ���б�־
    static WatermarkManager* s_pInstance; // ��̬ʵ��ָ�룬���ڴ��ڹ����з���
};