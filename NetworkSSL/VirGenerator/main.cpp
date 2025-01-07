#include <windows.h>
#include <iostream>
#include <fstream>

#include "resource.h"

#include "PEGenerator.h"

//int wmain(int argc, wchar_t* argv[]) {
//
//    CPEGenerator pe_gen;
//    pe_gen.ParseParams(argc, argv);
//
//    return 0;
//}

#ifdef _WINCMD

int wmain(int argc, wchar_t* argv[]) {
    setlocale(LC_ALL, "chs");
    CPEGenerator pe_gen;
    pe_gen.ParseParams(argc, argv);

    return 0;
}

#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    setlocale(LC_ALL, "chs");
    // 获取宽字符命令行参数
    LPWSTR cmdLine = GetCommandLineW();

    // 将命令行字符串解析为 `argc` 和 `argv` 风格
    int argc;
    LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);

    if (argv == NULL) {
        return 1;
    }
    CPEGenerator pe_gen;
    pe_gen.ParseParams(argc, argv);

    // 释放解析后的参数内存
    LocalFree(argv);

    return 0;
}
#endif
