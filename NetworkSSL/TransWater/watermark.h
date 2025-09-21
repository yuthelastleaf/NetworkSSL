
#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <GdiPlus.h>

// 显示器结构体，用于存储显示器信息
struct MonitorInfo {
    HMONITOR hMonitor;
    RECT rect;          // 显示器绝对区域（屏幕坐标）
    bool isPrimary;     // 是否为主显示器
};

// 水印参数结构体
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

// 改进的点阵水印参数结构体
struct ImprovedDotMatrixParams {
    std::string ipAddress;          // IP地址
    int dotRadius;                  // 圆点半径
    int dotSpacing;                 // 点阵间距
    int blockSpacing;               // 块间距
    COLORREF headerColor;           // 头部行颜色（用于区分同步行）
    COLORREF dataColor;             // 数据行颜色
    float opacity;                  // 透明度 (0.0-1.0)
    float rotation;                 // 旋转角度
    int hSpacing;                   // 水印间距
    int vSpacing;                   // 垂直间距
    bool showDebugInfo;             // 是否显示调试信息
    bool enableBlinking;            // 是否启用闪烁（IP变化时）
    int blinkDuration;              // 闪烁持续时间（毫秒）
};

class ImprovedWatermarkManager {
public:
    ImprovedWatermarkManager();
    ~ImprovedWatermarkManager();

    // 初始化
    bool Initialize(HINSTANCE hInstance);

    // 设置水印参数
    void SetWatermarkParams(const WatermarkParams& params);

    // 设置改进的点阵水印参数
    void SetImprovedDotMatrixParams(const ImprovedDotMatrixParams& params);

    // 开始显示水印
    bool StartWatermark();

    // 开始显示改进的点阵水印
    bool StartImprovedDotWatermark();

    // 停止显示水印
    void StopWatermark();

    // 更新水印
    void UpdateWatermark();

    // 更新点阵水印
    void UpdateDotWatermark();

    // 静态窗口过程函数
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // 窗口过程函数
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // 创建透明窗口
    HWND CreateTransparentWindow(int width, int height);

    // 获取所有显示器信息
    std::vector<MonitorInfo> GetAllMonitors();

    // 枚举显示器回调函数
    static BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

    // 绘制多显示器水印
    void DrawMultiMonitorWatermark();

    // 绘制多显示器改进点阵水印
    void DrawMultiMonitorImprovedDotWatermark();

    // 改进的点阵协议相关函数
    struct EncodedMatrix {
        std::vector<std::vector<bool>> matrix;
        std::string originalIP;
        uint8_t checksum;
        bool isValid;

        EncodedMatrix() : isValid(false), checksum(0) {
            matrix = std::vector<std::vector<bool>>(5, std::vector<bool>(8, false));
        }
    };

    // IP编码为改进协议
    EncodedMatrix EncodeIPToImprovedProtocol(const std::string& ip);

    // 创建改进的点阵图案
    void CreateImprovedDotMatrixPattern(HDC targetDC, int patternWidth, int patternHeight, const EncodedMatrix& encoded);

    // 解析IP地址
    std::vector<uint8_t> ParseIP(const std::string& ip);

    // 填充矩阵行
    void FillMatrixRow(std::vector<bool>& row, uint8_t value);

    // IP变化检测和闪烁控制
    void CheckIPChange();
    void StartBlinking();
    void StopBlinking();

    // 处理文本中的变量
    std::wstring ProcessTextVariables(const std::wstring& text);

    // 获取当前时间字符串
    std::wstring GetCurrentTimeString();

    // 获取主机名
    std::wstring GetHostName();

    // 获取IP地址
    std::string GetIPAddress();

    // 获取MAC地址
    std::wstring GetMACAddress();

    // 获取当前登录用户名
    std::wstring GetLoginName();

private:
    // 协议常量
    static const int BITS_PER_ROW = 8;
    static const int TOTAL_ROWS = 5;
    static const int HEADER_ROW = 0;
    static const int DATA_START_ROW = 1;
    static const uint8_t SYNC_PATTERN = 0xA0;  // 1010 0000
    static const uint8_t SYNC_MASK = 0xF0;
    static const uint8_t CHECKSUM_MASK = 0x0F;

    HWND m_hwnd;                                    // 窗口句柄
    HINSTANCE m_hInstance;                          // 程序实例句柄
    WatermarkParams m_params;                       // 水印参数
    ImprovedDotMatrixParams m_dotParams;           // 改进点阵水印参数
    const UINT_PTR TIMER_ID = 1;                   // 定时器ID
    const UINT_PTR BLINK_TIMER_ID = 2;             // 闪烁定时器ID
    ULONG_PTR m_gdiplusToken;                      // GDI+令牌
    bool m_initialized;                             // 初始化标志
    bool m_running;                                 // 运行标志
    bool m_isDotMode;                              // 是否为点阵模式
    bool m_isBlinking;                             // 是否正在闪烁
    int m_blinkCount;                              // 闪烁计数
    EncodedMatrix m_currentMatrix;                 // 当前编码矩阵
    std::string m_lastIP;                          // 上次检测的IP
    static ImprovedWatermarkManager* s_pInstance;  // 静态实例指针
};