
#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <GdiPlus.h>

// ��ʾ���ṹ�壬���ڴ洢��ʾ����Ϣ
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

// �Ľ��ĵ���ˮӡ�����ṹ��
struct ImprovedDotMatrixParams {
    std::string ipAddress;          // IP��ַ
    int dotRadius;                  // Բ��뾶
    int dotSpacing;                 // ������
    int blockSpacing;               // ����
    COLORREF headerColor;           // ͷ������ɫ����������ͬ���У�
    COLORREF dataColor;             // ��������ɫ
    float opacity;                  // ͸���� (0.0-1.0)
    float rotation;                 // ��ת�Ƕ�
    int hSpacing;                   // ˮӡ���
    int vSpacing;                   // ��ֱ���
    bool showDebugInfo;             // �Ƿ���ʾ������Ϣ
    bool enableBlinking;            // �Ƿ�������˸��IP�仯ʱ��
    int blinkDuration;              // ��˸����ʱ�䣨���룩
};

class ImprovedWatermarkManager {
public:
    ImprovedWatermarkManager();
    ~ImprovedWatermarkManager();

    // ��ʼ��
    bool Initialize(HINSTANCE hInstance);

    // ����ˮӡ����
    void SetWatermarkParams(const WatermarkParams& params);

    // ���øĽ��ĵ���ˮӡ����
    void SetImprovedDotMatrixParams(const ImprovedDotMatrixParams& params);

    // ��ʼ��ʾˮӡ
    bool StartWatermark();

    // ��ʼ��ʾ�Ľ��ĵ���ˮӡ
    bool StartImprovedDotWatermark();

    // ֹͣ��ʾˮӡ
    void StopWatermark();

    // ����ˮӡ
    void UpdateWatermark();

    // ���µ���ˮӡ
    void UpdateDotWatermark();

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

    // ���ƶ���ʾ���Ľ�����ˮӡ
    void DrawMultiMonitorImprovedDotWatermark();

    // �Ľ��ĵ���Э����غ���
    struct EncodedMatrix {
        std::vector<std::vector<bool>> matrix;
        std::string originalIP;
        uint8_t checksum;
        bool isValid;

        EncodedMatrix() : isValid(false), checksum(0) {
            matrix = std::vector<std::vector<bool>>(5, std::vector<bool>(8, false));
        }
    };

    // IP����Ϊ�Ľ�Э��
    EncodedMatrix EncodeIPToImprovedProtocol(const std::string& ip);

    // �����Ľ��ĵ���ͼ��
    void CreateImprovedDotMatrixPattern(HDC targetDC, int patternWidth, int patternHeight, const EncodedMatrix& encoded);

    // ����IP��ַ
    std::vector<uint8_t> ParseIP(const std::string& ip);

    // ��������
    void FillMatrixRow(std::vector<bool>& row, uint8_t value);

    // IP�仯������˸����
    void CheckIPChange();
    void StartBlinking();
    void StopBlinking();

    // �����ı��еı���
    std::wstring ProcessTextVariables(const std::wstring& text);

    // ��ȡ��ǰʱ���ַ���
    std::wstring GetCurrentTimeString();

    // ��ȡ������
    std::wstring GetHostName();

    // ��ȡIP��ַ
    std::string GetIPAddress();

    // ��ȡMAC��ַ
    std::wstring GetMACAddress();

    // ��ȡ��ǰ��¼�û���
    std::wstring GetLoginName();

private:
    // Э�鳣��
    static const int BITS_PER_ROW = 8;
    static const int TOTAL_ROWS = 5;
    static const int HEADER_ROW = 0;
    static const int DATA_START_ROW = 1;
    static const uint8_t SYNC_PATTERN = 0xA0;  // 1010 0000
    static const uint8_t SYNC_MASK = 0xF0;
    static const uint8_t CHECKSUM_MASK = 0x0F;

    HWND m_hwnd;                                    // ���ھ��
    HINSTANCE m_hInstance;                          // ����ʵ�����
    WatermarkParams m_params;                       // ˮӡ����
    ImprovedDotMatrixParams m_dotParams;           // �Ľ�����ˮӡ����
    const UINT_PTR TIMER_ID = 1;                   // ��ʱ��ID
    const UINT_PTR BLINK_TIMER_ID = 2;             // ��˸��ʱ��ID
    ULONG_PTR m_gdiplusToken;                      // GDI+����
    bool m_initialized;                             // ��ʼ����־
    bool m_running;                                 // ���б�־
    bool m_isDotMode;                              // �Ƿ�Ϊ����ģʽ
    bool m_isBlinking;                             // �Ƿ�������˸
    int m_blinkCount;                              // ��˸����
    EncodedMatrix m_currentMatrix;                 // ��ǰ�������
    std::string m_lastIP;                          // �ϴμ���IP
    static ImprovedWatermarkManager* s_pInstance;  // ��̬ʵ��ָ��
};