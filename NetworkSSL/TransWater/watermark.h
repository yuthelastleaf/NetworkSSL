#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <GdiPlus.h>
// #include <opencv2/opencv.hpp>

// 辅助结构体，用于存储显示器信息
struct MonitorInfo {
    HMONITOR hMonitor;
    RECT rect;          // 显示器矩形区域（屏幕坐标）
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

class WatermarkManager {
public:
    WatermarkManager();
    ~WatermarkManager();

    // 初始化
    bool Initialize(HINSTANCE hInstance);

    // 设置水印参数
    void SetWatermarkParams(const WatermarkParams& params);

    // 开始显示水印
    bool StartWatermark();

    // 停止显示水印
    void StopWatermark();

    // 更新水印
    void UpdateWatermark();

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

    
private:
    // 处理文本中的变量
    std::wstring ProcessTextVariables(const std::wstring& text);

    // 获取当前时间字符串 (格式: YYYY/MM/DD HH:MM)
    std::wstring GetCurrentTimeString();

    // 获取主机名
    std::wstring GetHostName();

    // 获取IP地址
    std::wstring GetIPAddress();

    // 获取MAC地址
    std::wstring GetMACAddress();

    // 获取当前登录用户名
    std::wstring GetLoginName();

private:
    HWND m_hwnd;                     // 窗口句柄
    HINSTANCE m_hInstance;           // 程序实例句柄
    WatermarkParams m_params;        // 水印参数
    const UINT_PTR TIMER_ID = 1;     // 定时器ID
    ULONG_PTR m_gdiplusToken;        // GDI+令牌
    bool m_initialized;              // 初始化标志
    bool m_running;                  // 运行标志
    static WatermarkManager* s_pInstance; // 静态实例指针，用于窗口过程中访问
};