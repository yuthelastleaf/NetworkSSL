#include "watermark.h"  // 使用改进的水印头文件
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <locale>
#include <iostream>

// 解析命令行参数，设置水印参数
WatermarkParams ParseCommandLine()
{
    WatermarkParams params;

    // 设置默认值
    params.text = L"水印文本";
    params.angle = 30;
    params.hSpacing = 150;
    params.vSpacing = 180;
    params.fontName = L"微软雅黑";
    params.fontSize = 20;
    params.textAlpha = 50;
    params.textColor = RGB(128, 128, 128);

    // 获取命令行参数
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (wcscmp(argv[i], L"-text") == 0 && i + 1 < argc) {
                params.text = argv[++i];
            }
            else if (wcscmp(argv[i], L"-angle") == 0 && i + 1 < argc) {
                params.angle = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-hspace") == 0 && i + 1 < argc) {
                params.hSpacing = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-vspace") == 0 && i + 1 < argc) {
                params.vSpacing = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-font") == 0 && i + 1 < argc) {
                params.fontName = argv[++i];
            }
            else if (wcscmp(argv[i], L"-size") == 0 && i + 1 < argc) {
                params.fontSize = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-alpha") == 0 && i + 1 < argc) {
                params.textAlpha = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-color") == 0 && i + 3 < argc) {
                int r = _wtoi(argv[++i]);
                int g = _wtoi(argv[++i]);
                int b = _wtoi(argv[++i]);
                params.textColor = RGB(r, g, b);
            }
        }

        LocalFree(argv);
    }

    return params;
}

// 解析改进的点阵水印命令行参数
ImprovedDotMatrixParams ParseImprovedDotMatrixCommandLine()
{
    ImprovedDotMatrixParams params;

    // 设置默认值 - 使用改进协议的默认参数
    params.ipAddress = "192.168.1.100";
    params.dotRadius = 3;
    params.dotSpacing = 18;
    params.blockSpacing = 120;
    params.headerColor = RGB(255, 0, 0);        // 红色头部行（同步+校验）
    params.dataColor = RGB(0, 100, 255);        // 蓝色数据行（IP数据）
    params.opacity = 0.6f;
    params.rotation = 15.0f;
    params.hSpacing = 150;
    params.vSpacing = 120;
    params.showDebugInfo = false;               // 默认不显示调试信息
    params.enableBlinking = true;               // 默认启用IP变化闪烁
    params.blinkDuration = 2000;                // 闪烁2秒

    // 获取命令行参数
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (wcscmp(argv[i], L"-ip") == 0 && i + 1 < argc) {
                // 转换宽字符串为多字节字符串
                char ip[32];
                WideCharToMultiByte(CP_UTF8, 0, argv[++i], -1, ip, sizeof(ip), NULL, NULL);
                params.ipAddress = ip;
            }
            else if (wcscmp(argv[i], L"-dotradius") == 0 && i + 1 < argc) {
                params.dotRadius = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-dotspacing") == 0 && i + 1 < argc) {
                params.dotSpacing = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-blockspacing") == 0 && i + 1 < argc) {
                params.blockSpacing = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-headercolor") == 0 && i + 3 < argc) {
                int r = _wtoi(argv[++i]);
                int g = _wtoi(argv[++i]);
                int b = _wtoi(argv[++i]);
                params.headerColor = RGB(r, g, b);
            }
            else if (wcscmp(argv[i], L"-datacolor") == 0 && i + 3 < argc) {
                int r = _wtoi(argv[++i]);
                int g = _wtoi(argv[++i]);
                int b = _wtoi(argv[++i]);
                params.dataColor = RGB(r, g, b);
            }
            else if (wcscmp(argv[i], L"-opacity") == 0 && i + 1 < argc) {
                params.opacity = (float)_wtof(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-rotation") == 0 && i + 1 < argc) {
                params.rotation = (float)_wtof(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-dhspace") == 0 && i + 1 < argc) {
                params.hSpacing = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-dvspace") == 0 && i + 1 < argc) {
                params.vSpacing = _wtoi(argv[++i]);
            }
            else if (wcscmp(argv[i], L"-debug") == 0) {
                params.showDebugInfo = true;
            }
            else if (wcscmp(argv[i], L"-noblink") == 0) {
                params.enableBlinking = false;
            }
            else if (wcscmp(argv[i], L"-blinktime") == 0 && i + 1 < argc) {
                params.blinkDuration = _wtoi(argv[++i]);
            }
        }

        LocalFree(argv);
    }

    return params;
}

// 检查是否使用点阵模式
bool IsDotMatrixMode()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    bool isDotMode = false;
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (wcscmp(argv[i], L"-dotmatrix") == 0 || wcscmp(argv[i], L"-improved") == 0) {
                isDotMode = true;
                break;
            }
        }
        LocalFree(argv);
    }

    return isDotMode;
}

// 显示帮助信息
void ShowUsage()
{
    const wchar_t* usage = L"水印程序使用说明\n\n"
        L"传统文字水印模式:\n"
        L"  -text <文本>        水印文本内容\n"
        L"  -angle <角度>       旋转角度\n"
        L"  -hspace <间距>      水平间距\n"
        L"  -vspace <间距>      垂直间距\n"
        L"  -font <字体名>      字体名称\n"
        L"  -size <大小>        字体大小\n"
        L"  -alpha <透明度>     透明度 (0-255)\n"
        L"  -color <R> <G> <B>  颜色值\n\n"
        L"改进点阵水印模式:\n"
        L"  -dotmatrix 或 -improved  启用改进点阵模式\n"
        L"  -ip <IP地址>        要编码的IP地址\n"
        L"  -dotradius <半径>   圆点半径\n"
        L"  -dotspacing <间距>  点阵间距\n"
        L"  -blockspacing <间距> 块间距\n"
        L"  -headercolor <R> <G> <B>  头部行颜色（红色推荐）\n"
        L"  -datacolor <R> <G> <B>    数据行颜色（蓝色推荐）\n"
        L"  -opacity <透明度>   透明度 (0.0-1.0)\n"
        L"  -rotation <角度>    旋转角度\n"
        L"  -dhspace <间距>     水平间距\n"
        L"  -dvspace <间距>     垂直间距\n"
        L"  -debug             显示调试信息\n"
        L"  -noblink           禁用IP变化闪烁\n"
        L"  -blinktime <毫秒>  闪烁持续时间\n\n"
        L"示例:\n"
        L"  watermark.exe                           # 默认文字水印\n"
        L"  watermark.exe -dotmatrix -ip 192.168.1.1 -debug  # 改进点阵水印\n"
        L"  watermark.exe -improved -ip 10.0.0.1 -headercolor 255 100 0  # 橙色头部";

    MessageBox(NULL, usage, L"使用说明", MB_ICONINFORMATION);
}

// 检查是否显示帮助
bool ShouldShowHelp()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    bool showHelp = false;
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (wcscmp(argv[i], L"-help") == 0 ||
                wcscmp(argv[i], L"-h") == 0 ||
                wcscmp(argv[i], L"/?") == 0) {
                showHelp = true;
                break;
            }
        }
        LocalFree(argv);
    }

    return showHelp;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 设置区域设置，支持中文
    setlocale(LC_ALL, "chs");
    _wsetlocale(LC_ALL, L"chs");

    // 检查是否显示帮助信息
    if (ShouldShowHelp()) {
        ShowUsage();
        return 0;
    }

    // 创建改进的水印管理器
    ImprovedWatermarkManager watermarkManager;

    // 初始化
    if (!watermarkManager.Initialize(hInstance)) {
        MessageBox(NULL, L"初始化失败", L"错误", MB_ICONERROR);
        return 1;
    }

    // 检查是否使用改进的点阵模式
    if (IsDotMatrixMode()) {
        // ===== 改进的点阵水印模式 =====
        ImprovedDotMatrixParams dotParams = ParseImprovedDotMatrixCommandLine();
        watermarkManager.SetImprovedDotMatrixParams(dotParams);

        // 开始显示改进的点阵水印
        if (!watermarkManager.StartImprovedDotWatermark()) {
            MessageBox(NULL, L"启动改进点阵水印失败", L"错误", MB_ICONERROR);
            return 1;
        }

        // 显示详细信息
        wchar_t infoMsg[512];
        swprintf_s(infoMsg,
            L"🔥 改进点阵水印已启动 🔥\n\n"
            L"📡 IP地址: %hs\n"
            L"🔴 头部行: 红色（同步+校验）\n"
            L"🔵 数据行: 蓝色（IP数据）\n"
            L"📏 点大小: %d像素\n"
            L"📐 点间距: %d像素\n"
            L"💫 透明度: %.1f%%\n"
            L"🔄 旋转角度: %.1f°\n"
            L"🔍 调试模式: %s\n"
            L"✨ 变化闪烁: %s\n\n"
            L"🎯 新协议特性:\n"
            L"• 头部行包含同步位(1010) + 校验和\n"
            L"• 数据行包含4字节IP地址\n"
            L"• 支持IP变化自动检测和闪烁提醒\n"
            L"• 大幅提升识别准确率",
            dotParams.ipAddress.c_str(),
            dotParams.dotRadius * 2,
            dotParams.dotSpacing,
            dotParams.opacity * 100,
            dotParams.rotation,
            dotParams.showDebugInfo ? L"开启" : L"关闭",
            dotParams.enableBlinking ? L"开启" : L"关闭");

        MessageBox(NULL, infoMsg, L"改进点阵水印信息", MB_ICONINFORMATION);

        // 控制台输出详细信息（如果有控制台的话）
        std::wcout << L"\n=== 改进点阵水印启动成功 ===" << std::endl;
        std::wcout << L"协议版本: 改进协议 v2.0" << std::endl;
        std::wcout << L"编码格式: 头部行(同步+校验) + 4字节IP数据" << std::endl;
        std::wcout << L"同步模式: 1010xxxx (固定)" << std::endl;
        std::wcout << L"视觉标识: 红色头部行 + 蓝色数据行" << std::endl;
        std::wcout << L"智能功能: IP变化检测 + 闪烁提醒" << std::endl;
    }
    else {
        // ===== 传统文字水印模式 =====
        WatermarkParams params = ParseCommandLine();
        watermarkManager.SetWatermarkParams(params);

        // 开始显示文字水印
        if (!watermarkManager.StartWatermark()) {
            MessageBox(NULL, L"启动文字水印失败", L"错误", MB_ICONERROR);
            return 1;
        }

        // 显示文字水印信息
        wchar_t infoMsg[256];
        swprintf_s(infoMsg,
            L"📝 文字水印已启动\n\n"
            L"📄 文本: %s\n"
            L"📐 角度: %d°\n"
            L"📏 字体: %s (%dpt)\n"
            L"💫 透明度: %d/255\n"
            L"📍 间距: %d x %d",
            params.text.c_str(),
            params.angle,
            params.fontName.c_str(),
            params.fontSize,
            params.textAlpha,
            params.hSpacing,
            params.vSpacing);

        MessageBox(NULL, infoMsg, L"文字水印信息", MB_ICONINFORMATION);
    }

    // 创建系统托盘图标（可选）
    // 这里可以添加托盘图标功能，方便用户控制水印

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// 可选：添加托盘图标功能
class TrayIcon {
private:
    NOTIFYICONDATA nid;
    HWND hwnd;
    static const UINT WM_TRAYICON = WM_USER + 1;
    static const UINT ID_TRAY_EXIT = 1001;
    static const UINT ID_TRAY_TOGGLE = 1002;
    static const UINT ID_TRAY_ABOUT = 1003;

public:
    TrayIcon(HWND window) : hwnd(window) {
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcscpy_s(nid.szTip, L"改进水印系统 - 正在运行");

        Shell_NotifyIcon(NIM_ADD, &nid);
    }

    ~TrayIcon() {
        Shell_NotifyIcon(NIM_DELETE, &nid);
    }

    void ShowContextMenu(int x, int y) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, ID_TRAY_ABOUT, L"关于水印系统");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, ID_TRAY_TOGGLE, L"切换显示/隐藏");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出");

        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }

    static LRESULT HandleTrayMessage(WPARAM wParam, LPARAM lParam) {
        if (wParam == 1) {  // 托盘图标ID
            switch (lParam) {
            case WM_RBUTTONUP: {
                POINT pt;
                GetCursorPos(&pt);
                // 这里需要创建TrayIcon实例来调用ShowContextMenu
                break;
            }
            case WM_LBUTTONDBLCLK:
                MessageBox(NULL, L"改进水印系统正在运行\n支持新的点阵协议", L"水印系统", MB_ICONINFORMATION);
                break;
            }
        }
        return 0;
    }
};